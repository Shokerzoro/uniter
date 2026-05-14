#ifndef KAFKACONNECTOR_H
#define KAFKACONNECTOR_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include "../contract/unitermessage.h"

namespace net {

/*
 * KafkaConnector — заглушка consumer-канала Kafka.
 *
 * Роль в целевой архитектуре (см. docs/fsm_appmanager.md и uniter-target-state):
 *   — одностороннее broadcast-чтение подтверждённых CUD операций от сервера;
 *   — хранение offset последнего обработанного сообщения в OS Secure Storage;
 *   — после подтверждения актуальности offset сервером — подписка на топик.
 *
 * В stub-варианте KafkaConnector:
 *   — onInitConnection(hash): через event-loop отдаёт произвольный offset.
 *   — onSubscribeKafka(): no-op + signalKafkaSubscribed(true) через event-loop.
 *   — onShutdown(): сбрасывает состояние.
 *   — onServerNotification(msg): вход от ServerConnector (TEMP connect) —
 *     принимает CUD-сообщения как имитацию broadcast-очереди Kafka,
 *     пересылает их вверх (AppManager → DataManager) как signalRecvMessage.
 *     Допустимые типы: UniterMessage с CrudAction != NOTCRUD и
 *     MessageStatus == NOTIFICATION.
 *
 * Реальная реализация (librdkafka + OS Secure Storage) — отдельный этап.
 */
class KafkaConnector : public QObject
{
    Q_OBJECT

private:
    // Синглтон
    KafkaConnector();

    KafkaConnector(const KafkaConnector&) = delete;
    KafkaConnector& operator=(const KafkaConnector&) = delete;
    KafkaConnector(KafkaConnector&&) = delete;
    KafkaConnector& operator=(KafkaConnector&&) = delete;

    bool m_initialized = false;
    bool m_subscribed  = false;
    QByteArray m_userhash;

public:
    static KafkaConnector* instance();
    ~KafkaConnector() override = default;

public slots:
    // От AppManager (вход в KCONNECTOR): инициализироваться под пользователя
    // и прислать сохранённый offset.
    void onInitConnection(QByteArray userhash);

    // От AppManager (вход в READY): подписаться на broadcast-топик Kafka.
    void onSubscribeKafka();

    // От AppManager: общий shutdown/logout — сбросить состояние.
    void onShutdown();

    // TEMP connect: приходит от ServerConnector после подтверждения CUD.
    // Пересылает сообщение вверх, только если оно удовлетворяет инварианту
    // broadcast-очереди: CrudAction != NOTCRUD и status == NOTIFICATION.
    // При инициализации мы ещё не подписаны — сообщения игнорируются
    // (в реальной системе их не существует до подписки).
    // Удалить при переходе на реальный Kafka.
    void onServerNotification(std::shared_ptr<contract::UniterMessage> message);

signals:
    // offset последнего обработанного сообщения Kafka (после инициализации).
    // Именно этот сигнал триггерит переход KCONNECTOR → KAFKA в FSM.
    void signalOffsetReady(QString offset);

    // Статус подписки на broadcast-топик (после READY).
    void signalKafkaSubscribed(bool ok);

    // Входящее UniterMessage из Kafka (CRUD / NOTIFICATION) → AppManager.
    // В целевой системе этот сигнал получает AppManager и через свой слот
    // onRecvUniterMessage маршрутизирует его в DataManager.
    void signalRecvMessage(std::shared_ptr<contract::UniterMessage> message);
};

} // namespace net

#endif // KAFKACONNECTOR_H
