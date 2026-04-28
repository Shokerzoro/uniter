
#ifndef SERVERCONNECTOR_H
#define SERVERCONNECTOR_H

#include <QObject>
#include <memory>
#include "../contract/unitermessage.h"

namespace uniter::net {

/*
 * ServerConnector - stub of the main server (authorization + CRUD backend).
 *
 * Role in target architecture (see uniter-target-state):
 * — TCP/SSL connection with a self-written server.
 * — Serialization of UniterMessage in XML and sending to the server.
 * - Deserialization of XML responses in UniterMessage.
 * — Buffering of outgoing CUD requests when there is no connection.
 * — Processing request timeouts.
 *
 * The implementation in its current form is a local stub for integration testing:
 * — Connection is instant and successful.
 * — Authorization is always successful; a user with all subsystems and full rights.
 * — When requesting relevance offset (GET_KAFKA_CREDENTIALS) — always “offset_actual=true”.
 * — Any CUD request is answered with RESPONSE/SUCCESS and additionally forwarded
 * a copy of the message (MessageStatus::NOTIFICATION) via KafkaConnector - simulating
 * broadcast about a confirmed modification. For this purpose, a temporary
 * direct signal/slot connect ServerConnector → KafkaConnector (main.cpp).
 * — On GET_MINIO_PRESIGNED_URL delegates to MinIOConnector (also through temporary
 * direct connect) and returns the response UniterMessage via signalRecvMessage.
 * — FULL_SYNC — confirms instantly as SUCCESS.
 */
class ServerConnector : public QObject
{
    Q_OBJECT

private:
    // Private constructor for singleton
    ServerConnector();

    // Prohibition of copying and moving
    ServerConnector(const ServerConnector&) = delete;
    ServerConnector& operator=(const ServerConnector&) = delete;
    ServerConnector(ServerConnector&&) = delete;
    ServerConnector& operator=(ServerConnector&&) = delete;

    bool connected_ = false;

    // Counters for debugging
    int seq_id_sent_ = 0;
    int seq_id_received_ = 0;

public:
    // Public static method to get an instance
    static ServerConnector* instance();

    ~ServerConnector() override;

public slots:
    // Command from AppManager: try to establish a connection.
    // In the stub version we always connect successfully and instantly.
    void onMakeConnection();

    // Command from AppManager: send message to "server".
    void onSendMessage(std::shared_ptr<contract::UniterMessage> message);

    // Shutdown (if you need to pull explicitly)
    void onShutdown();

    // TEMP connect: comes from MinIOConnector - ready presigned URL
    // for object_key. It is converted to RESPONSE and goes up (to AppManager).
    // Delete when switching to a real network - the server will give the URL itself.
    void onMinioPresignedUrlReady(std::shared_ptr<contract::UniterMessage> requestCopy,
                                  QString object_key,
                                  QString presigned_url);

signals:
    // Network → AppManager
    void signalConnectionUpdated(bool state);

    // Network → AppManager: incoming message (RESPONSE, ERROR, SUCCESS)
    void signalRecvMessage(std::shared_ptr<contract::UniterMessage> message);

    // TEMP: ServerConnector → KafkaConnector - after CUD confirmation
    // the server imitates a broadcast, duplicating the message as NOTIFICATION.
    // Delete when switching to a real network - the real server itself will be
    // send NOTIFICATION to Kafka without client intervention.
    void signalEmitKafkaNotification(std::shared_ptr<contract::UniterMessage> notification);

    // TEMP: ServerConnector → MinIOConnector - request presigned URL by object_key.
    // Delete when switching to a real network - the server itself communicates with MinIO.
    void signalRequestMinioPresignedUrl(std::shared_ptr<contract::UniterMessage> requestCopy,
                                        QString object_key,
                                        QString minio_operation);
};

} // namespace uniter::net

#endif // SERVERCONNECTOR_H
