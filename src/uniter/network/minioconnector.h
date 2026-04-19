#ifndef MINIOCONNECTOR_H
#define MINIOCONNECTOR_H

#include <QObject>
#include <QString>
#include <memory>
#include "../contract/unitermessage.h"

namespace uniter::net {

/*
 * MinIOConnector — заглушка.
 *
 * Роль в архитектуре (см. docs и uniter-target-state):
 *   — в рабочем режиме отвечает за операции с MinIO S3 и за consumer-канал
 *     Kafka (offset хранится в OS Secure Storage, привязан к пользователю);
 *   — на текущем этапе нас интересует только один сценарий: при входе FSM в
 *     состояние KAFKA AppManager просит MinIOConnector проинициализироваться;
 *     в ответ MinIOConnector должен прислать последний известный offset
 *     через сигнал signalOffsetReady(QString offset).
 *
 * Stub-поведение:
 *   — onInitConnection(): синхронно (через emit в тот же event-loop-слот)
 *     отдаёт произвольный offset. Это позволяет FSM продвигаться дальше
 *     без реальной сети.
 *   — onSubscribeKafka(): логирование + no-op; реальная подписка появится
 *     при имплементации KafkaConnector.
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

    bool m_initialized = false;

public:
    static MinIOConnector* instance();
    ~MinIOConnector() override = default;

public slots:
    // От AppManager: инициализироваться и вернуть offset.
    void onInitConnection();

    // От AppManager: подписаться на broadcast-топик Kafka. Пока no-op.
    void onSubscribeKafka();

    // От AppManager: общий shutdown/logout — сбросить состояние.
    void onShutdown();

signals:
    // offset последнего обработанного сообщения Kafka
    void signalOffsetReady(QString offset);

    // Статус подписки (для будущих состояний/диагностики)
    void signalKafkaSubscribed(bool ok);
};

} // namespace uniter::net

#endif // MINIOCONNECTOR_H
