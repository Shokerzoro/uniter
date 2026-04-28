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
 * MinIOConnector is a stub for the MinIO HTTP client.
 *
 * Role in target architecture (see uniter-target-state):
 * — HTTP GET/PUT/DELETE to MinIO using a presigned URL from the server.
 * — Work only with temporary presigned URLs, credentials are not stored.
 *
 * The Stub implementation runs locally:
 * — Uses the <workdir>/minio_local directory as a dummy S3 bucket
 * (created during initialization if it does not exist).
 * — Stores std::map<object_key → presigned_url> in memory. Presigned URL
 * in stub it has the format "file://<bucket_root>/<object_key>".
 * — When requesting a presigned URL by object_key (from ServerConnector via
 * TEMP signal/slot connect) returns an existing URL or creates
 * new if the key is not in map.
 * — With a GET request (from AppManager — UniterMessage PROTOCOL/GET_MINIO_FILE
 * REQUEST with add_data["presigned_url"]) returns local file path
 * via RESPONSE add_data["object_key","local_path"]. If there is no file -
 *     ERROR.
 * — With a PUT request (PROTOCOL/PUT_MINIO_FILE REQUEST with
 * add_data["presigned_url", "object_key", "local_path"]) copies the file
 * into the bucket along the path corresponding to the presigned URL and responds
 * RESPONSE/SUCCESS or ERROR.
 *
 * Communication with ServerConnector (TEMP signal/slot connect, main.cpp):
 *   ServerConnector → signalRequestMinioPresignedUrl → MinIOConnector::onRequestPresignedUrl
 *   MinIOConnector  → signalPresignedUrlReady       → ServerConnector::onMinioPresignedUrlReady
 *
 * Communication with AppManager (the main path, remains in the real system):
 *   AppManager      → signalSendUniterMessage → MinIOConnector::onSendMessage
 *   MinIOConnector  → signalRecvMessage       → AppManager::onRecvUniterMessage
 */
class MinIOConnector : public QObject
{
    Q_OBJECT

private:
    // Singleton
    MinIOConnector();

    MinIOConnector(const MinIOConnector&) = delete;
    MinIOConnector& operator=(const MinIOConnector&) = delete;
    MinIOConnector(MinIOConnector&&) = delete;
    MinIOConnector& operator=(MinIOConnector&&) = delete;

    // Local bucket (the folder in which MinIO objects “live”).
    QDir m_bucket;

    // object_key → presigned URL (in stub: file://<bucket>/<object_key>).
    std::map<QString, QString> m_presignedByKey;

    // Creates a new map entry for object_key and returns a URL.
    QString generatePresignedUrl(const QString& object_key);

    // Converts a presigned URL from stub format to a local path.
    // Returns an empty string if the format is wrong.
    QString presignedUrlToLocalPath(const QString& presigned_url) const;

public:
    static MinIOConnector* instance();
    ~MinIOConnector() override = default;

    // For testing/debugging - the path to the local bucket.
    QString bucketPath() const { return m_bucket.absolutePath(); }

public slots:
    // TEMP connect (ServerConnector → MinIOConnector).
    // Returns the presigned URL for object_key: if the key already exists - existing,
    // otherwise it creates a new one. Replies via signalPresignedUrlReady.
    void onRequestPresignedUrl(std::shared_ptr<contract::UniterMessage> requestCopy,
                               QString object_key,
                               QString minio_operation);

    // Main slot: AppManager → MinIOConnector.
    // Filters only GET_MINIO_FILE and PUT_MINIO_FILE; everything else
    // ignored.
    void onSendMessage(std::shared_ptr<contract::UniterMessage> message);

    // Resetting the state (to shutdown/logout).
    void onShutdown();

signals:
    // TEMP connect (MinIOConnector → ServerConnector).
    void signalPresignedUrlReady(std::shared_ptr<contract::UniterMessage> requestCopy,
                                 QString object_key,
                                 QString presigned_url);

    // Answers on GET/PUT operations for AppManager.
    void signalRecvMessage(std::shared_ptr<contract::UniterMessage> message);
};

} // namespace uniter::net

#endif // MINIOCONNECTOR_H
