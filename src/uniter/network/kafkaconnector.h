#ifndef KAFKACONNECTOR_H
#define KAFKACONNECTOR_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include "../contract/unitermessage.h"

namespace uniter::net {

/*
 * KafkaConnector - Kafka consumer channel stub.
 *
 * Role in target architecture (see docs/fsm_appmanager.md and uniter-target-state):
 * — one-way broadcast reading of confirmed CUD operations from the server;
 * — storing the offset of the last processed message in OS Secure Storage;
 * — after confirming the relevance of offset by the server — subscription to the topic.
 *
 * In the stub version of KafkaConnector:
 * — onInitConnection(hash): gives an arbitrary offset via event-loop.
 * - onSubscribeKafka(): no-op + signalKafkaSubscribed(true) via event-loop.
 * - onShutdown(): resets the state.
 * - onServerNotification(msg): input from ServerConnector (TEMP connect) -
 * accepts CUD messages as an imitation of a Kafka broadcast queue,
 * forwards them up (AppManager -> DataManager) as signalRecvMessage.
 * Valid types: UniterMessage with CrudAction != NOTCRUD and
 *     MessageStatus == NOTIFICATION.
 *
 * The actual implementation (librdkafka + OS Secure Storage) is a separate stage.
 */
class KafkaConnector : public QObject
{
    Q_OBJECT

private:
    // Singleton
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
    // From AppManager (login to KCONNECTOR): initialize for user
    // and send the saved offset.
    void onInitConnection(QByteArray userhash);

    // From AppManager (login to READY): subscribe to the Kafka broadcast topic.
    void onSubscribeKafka();

    // From AppManager: general shutdown/logout - reset state.
    void onShutdown();

    // TEMP connect: comes from ServerConnector after CUD confirmation.
    // Forwards a message up only if it satisfies the invariant
    // broadcast queues: CrudAction != NOTCRUD and status == NOTIFICATION.
    // During initialization, we are not yet subscribed - messages are ignored
    // (in a real system they do not exist until subscription).
    // Delete when migrating to real Kafka.
    void onServerNotification(std::shared_ptr<contract::UniterMessage> message);

signals:
    // offset of the last Kafka message processed (after initialization).
    // It is this signal that triggers the KCONNECTOR → KAFKA transition in FSM.
    void signalOffsetReady(QString offset);

    // Subscription status for the broadcast topic (after READY).
    void signalKafkaSubscribed(bool ok);

    // Incoming UniterMessage from Kafka (CRUD/NOTIFICATION) → AppManager.
    // On the target system, this signal is received by AppManager and through its slot
    // onRecvUniterMessage routes it to the DataManager.
    void signalRecvMessage(std::shared_ptr<contract::UniterMessage> message);
};

} // namespace uniter::net

#endif // KAFKACONNECTOR_H
