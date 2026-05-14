
#ifndef SERVERCONNECTOR_H
#define SERVERCONNECTOR_H

#include <QObject>
#include <memory>
#include "../contract/unitermessage.h"

namespace net {

/*
 * ServerConnector — заглушка основного сервера (authorization + CRUD backend).
 *
 * Роль в целевой архитектуре (см. uniter-target-state):
 *   — TCP/SSL соединение с самописным сервером.
 *   — Сериализация UniterMessage в XML и отправка на сервер.
 *   — Десериализация XML-ответов в UniterMessage.
 *   — Буферизация исходящих CUD запросов при отсутствии соединения.
 *   — Обработка таймаутов запросов.
 *
 * Реализация в текущем виде — локальная заглушка для интеграционного тестирования:
 *   — Подключение мгновенное, успешное.
 *   — Авторизация всегда успешна; пользователь со всеми подсистемами и полными правами.
 *   — На запрос актуальности offset (GET_KAFKA_CREDENTIALS) — всегда "offset_actual=true".
 *   — На любой CUD-запрос отвечает RESPONSE/SUCCESS и дополнительно пересылает
 *     копию сообщения (MessageStatus::NOTIFICATION) через KafkaConnector — имитируя
 *     broadcast о подтверждённой модификации. Для этого задействован временный
 *     прямой signal/slot connect ServerConnector → KafkaConnector (main.cpp).
 *   — На GET_MINIO_PRESIGNED_URL делегирует MinIOConnector-у (также через временный
 *     прямой connect) и возвращает ответный UniterMessage через signalRecvMessage.
 *   — FULL_SYNC — подтверждает мгновенно как SUCCESS.
 */
class ServerConnector : public QObject
{
    Q_OBJECT

private:
    // Приватный конструктор для синглтона
    ServerConnector();

    // Запрет копирования и перемещения
    ServerConnector(const ServerConnector&) = delete;
    ServerConnector& operator=(const ServerConnector&) = delete;
    ServerConnector(ServerConnector&&) = delete;
    ServerConnector& operator=(ServerConnector&&) = delete;

    bool connected_ = false;

    // Счётчики для отладки
    int seq_id_sent_ = 0;
    int seq_id_received_ = 0;

public:
    // Публичный статический метод получения экземпляра
    static ServerConnector* instance();

    ~ServerConnector() override;

public slots:
    // Команда от AppManager: попытаться установить соединение.
    // В stub-варианте подключаемся всегда успешно и мгновенно.
    void onMakeConnection();

    // Команда от AppManager: отправить сообщение на «сервер».
    void onSendMessage(std::shared_ptr<contract::UniterMessage> message);

    // Завершение работы (если нужно дергать явно)
    void onShutdown();

    // TEMP connect: приходит из MinIOConnector — готовый presigned URL
    // для object_key. Преобразуется в RESPONSE и уходит вверх (в AppManager).
    // Удалить при переходе на реальную сеть — сервер будет отдавать URL сам.
    void onMinioPresignedUrlReady(std::shared_ptr<contract::UniterMessage> requestCopy,
                                  QString object_key,
                                  QString presigned_url);

signals:
    // Network → AppManager
    void signalConnectionUpdated(bool state);

    // Network → AppManager: входящее сообщение (RESPONSE, ERROR, SUCCESS)
    void signalRecvMessage(std::shared_ptr<contract::UniterMessage> message);

    // TEMP: ServerConnector → KafkaConnector — после подтверждения CUD
    // сервер имитирует broadcast, дублируя сообщение как NOTIFICATION.
    // Удалить при переходе на реальную сеть — настоящий сервер будет сам
    // отправлять NOTIFICATION в Kafka без вмешательства клиента.
    void signalEmitKafkaNotification(std::shared_ptr<contract::UniterMessage> notification);

    // TEMP: ServerConnector → MinIOConnector — запрос presigned URL по object_key.
    // Удалить при переходе на реальную сеть — сервер сам общается с MinIO.
    void signalRequestMinioPresignedUrl(std::shared_ptr<contract::UniterMessage> requestCopy,
                                        QString object_key,
                                        QString minio_operation);
};

} // namespace net

#endif // SERVERCONNECTOR_H
