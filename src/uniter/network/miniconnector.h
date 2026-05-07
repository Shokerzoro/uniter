#ifndef MINIOCONNECTOR_H
#define MINIOCONNECTOR_H

#include <QObject>
#include <QString>
#include <QDir>
#include <map>
#include <memory>
#include "../contract/unitermessage.h"

namespace uniter::net {

/*
 * MinIOConnector — заглушка HTTP-клиента MinIO.
 *
 * Роль в целевой архитектуре (см. uniter-target-state):
 *   — HTTP GET/PUT/DELETE к MinIO по presigned URL от сервера.
 *   — Работа только с временными presigned URL, credentials не хранятся.
 *
 * Stub-реализация работает локально:
 *   — Использует каталог <workdir>/minio_local как фиктивный S3 bucket
 *     (создаётся при инициализации, если его нет).
 *   — Хранит std::map<object_key → presigned_url> в памяти. Presigned URL
 *     в stub имеет формат "file://<bucket_root>/<object_key>".
 *   — При запросе presigned URL по object_key (от ServerConnector через
 *     TEMP signal/slot connect) возвращает существующий URL или создаёт
 *     новый, если ключа нет в map.
 *   — При GET-запросе (от AppManager — UniterMessage PROTOCOL/GET_MINIO_FILE
 *     REQUEST с add_data["presigned_url"]) возвращает локальный путь файла
 *     через RESPONSE add_data["object_key","local_path"]. Если файла нет —
 *     ERROR.
 *   — При PUT-запросе (PROTOCOL/PUT_MINIO_FILE REQUEST с
 *     add_data["presigned_url", "object_key", "local_path"]) копирует файл
 *     в bucket по пути, соответствующему presigned URL, и отвечает
 *     RESPONSE/SUCCESS либо ERROR.
 *
 * Связь с ServerConnector (TEMP signal/slot connect, main.cpp):
 *   ServerConnector → signalRequestMinioPresignedUrl → MinIOConnector::onRequestPresignedUrl
 *   MinIOConnector  → signalPresignedUrlReady       → ServerConnector::onMinioPresignedUrlReady
 *
 * Связь с AppManager (основной путь, остаётся и в реальной системе):
 *   AppManager      → signalSendUniterMessage → MinIOConnector::onSendMessage
 *   MinIOConnector  → signalRecvMessage       → AppManager::onRecvUniterMessage
 */
class MinIOConnector : public QObject
{
    Q_OBJECT

private:
    // Синглтон
    MinIOConnector();

    MinIOConnector(const MinIOConnector&) = delete;
    MinIOConnector& operator=(const MinIOConnector&) = delete;
    MinIOConnector(MinIOConnector&&) = delete;
    MinIOConnector& operator=(MinIOConnector&&) = delete;

    // Локальный bucket (папка, в которой «живут» объекты MinIO).
    QDir m_bucket;

    // object_key → presigned URL (в stub: file://<bucket>/<object_key>).
    std::map<QString, QString> m_presignedByKey;

    // Создаёт новую запись в map для object_key и возвращает URL.
    QString generatePresignedUrl(const QString& object_key);

    // Преобразует presigned URL из stub-формата в локальный путь.
    // Возвращает пустую строку, если формат не тот.
    QString presignedUrlToLocalPath(const QString& presigned_url) const;

public:
    static MinIOConnector* instance();
    ~MinIOConnector() override = default;

    // Для тестов/отладки — путь к локальному bucket.
    QString bucketPath() const { return m_bucket.absolutePath(); }

public slots:
    // TEMP connect (ServerConnector → MinIOConnector).
    // Возвращает presigned URL для object_key: если ключ уже есть — существующий,
    // иначе создаёт новый. Отвечает через signalPresignedUrlReady.
    void onRequestPresignedUrl(std::shared_ptr<contract::UniterMessage> requestCopy,
                               QString object_key,
                               QString minio_operation);

    // Основной слот: AppManager → MinIOConnector.
    // Фильтрует только GET_MINIO_FILE и PUT_MINIO_FILE; всё остальное
    // игнорируется.
    void onSendMessage(std::shared_ptr<contract::UniterMessage> message);

    // Сброс состояния (на shutdown/logout).
    void onShutdown();

signals:
    // TEMP connect (MinIOConnector → ServerConnector).
    void signalPresignedUrlReady(std::shared_ptr<contract::UniterMessage> requestCopy,
                                 QString object_key,
                                 QString presigned_url);

    // Ответы по операциям GET/PUT для AppManager.
    void signalRecvMessage(std::shared_ptr<contract::UniterMessage> message);
};

} // namespace uniter::net

#endif // MINIOCONNECTOR_H
