#include "miniconnector.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

namespace uniter::net {

using namespace uniter::contract;

MinIOConnector* MinIOConnector::instance()
{
    static MinIOConnector instance;
    return &instance;
}

MinIOConnector::MinIOConnector()
    : QObject(nullptr)
{
    // Local bucket: <application>/minio_local.
    // In the stub version, we store it next to the executable file so as not to require
    // from the user of rights to directories outside the working one.
    const QString root = QCoreApplication::applicationDirPath().isEmpty()
                         ? QDir::currentPath()
                         : QCoreApplication::applicationDirPath();
    m_bucket = QDir(root);
    if (!m_bucket.exists(QStringLiteral("minio_local"))) {
        m_bucket.mkpath(QStringLiteral("minio_local"));
    }
    m_bucket.cd(QStringLiteral("minio_local"));

    qDebug() << "MinIOConnector: local bucket initialized at" << m_bucket.absolutePath();
}

QString MinIOConnector::generatePresignedUrl(const QString& object_key)
{
    // Stub format: file://<bucket_absolute_path>/<object_key>
    // A real presigned URL would have a temporary token and expiration.
    const QString path = m_bucket.absoluteFilePath(object_key);
    const QString url  = QStringLiteral("file://") + path;
    m_presignedByKey[object_key] = url;
    qDebug() << "MinIOConnector: generated presigned URL for key=" << object_key
             << "url=" << url;
    return url;
}

QString MinIOConnector::presignedUrlToLocalPath(const QString& presigned_url) const
{
    // In stub format: "file://<absolute path>".
    const QString prefix = QStringLiteral("file://");
    if (!presigned_url.startsWith(prefix)) {
        return {};
    }
    return presigned_url.mid(prefix.size());
}

void MinIOConnector::onRequestPresignedUrl(std::shared_ptr<contract::UniterMessage> requestCopy,
                                           QString object_key,
                                           QString minio_operation)
{
    Q_UNUSED(minio_operation); // in stub the operation does not affect the URL
    if (object_key.isEmpty()) {
        qDebug() << "MinIOConnector::onRequestPresignedUrl() — empty object_key, skip";
        // We still return an empty URL - the server will treat it as an error.
        emit signalPresignedUrlReady(requestCopy, object_key, QString());
        return;
    }

    QString url;
    auto it = m_presignedByKey.find(object_key);
    if (it != m_presignedByKey.end()) {
        url = it->second;
        qDebug() << "MinIOConnector: existing URL for key=" << object_key;
    } else {
        url = generatePresignedUrl(object_key);
    }

    emit signalPresignedUrlReady(requestCopy, object_key, url);
}

void MinIOConnector::onSendMessage(std::shared_ptr<contract::UniterMessage> message)
{
    if (!message) return;

    // We are only interested in GET_MINIO_FILE and PUT_MINIO_FILE in the REQUEST status.
    if (message->subsystem != Subsystem::PROTOCOL ||
        message->status    != MessageStatus::REQUEST ||
        (message->protact != ProtocolAction::GET_MINIO_FILE &&
         message->protact != ProtocolAction::PUT_MINIO_FILE))
    {
        return;
    }

    const bool is_put = (message->protact == ProtocolAction::PUT_MINIO_FILE);

    QString presigned_url;
    QString object_key;
    QString local_path_in;

    {
        auto it = message->add_data.find("presigned_url");
        if (it != message->add_data.end()) presigned_url = QString::fromStdString(it->second);
    }
    {
        auto it = message->add_data.find("object_key");
        if (it != message->add_data.end()) object_key = QString::fromStdString(it->second);
    }
    {
        auto it = message->add_data.find("local_path");
        if (it != message->add_data.end()) local_path_in = QString::fromStdString(it->second);
    }

    const QString bucket_path = presignedUrlToLocalPath(presigned_url);
    qDebug() << "MinIOConnector::onSendMessage() op=" << (is_put ? "PUT" : "GET")
             << "key=" << object_key
             << "url=" << presigned_url
             << "bucket_path=" << bucket_path;

    // We generate the response based on the original request to save the correlation.
    auto response = std::make_shared<UniterMessage>(*message);
    response->status  = MessageStatus::RESPONSE;
    response->add_data.clear();
    response->add_data.emplace("object_key",    object_key.toStdString());
    response->add_data.emplace("presigned_url", presigned_url.toStdString());

    if (bucket_path.isEmpty()) {
        response->status = MessageStatus::ERROR;
        response->error  = ErrorCode::BAD_REQUEST;
        response->add_data.emplace("reason", std::string("bad presigned_url"));
        emit signalRecvMessage(response);
        return;
    }

    if (is_put) {
        // PUT: copy the file via local_path_in → bucket_path.
        if (local_path_in.isEmpty()) {
            response->status = MessageStatus::ERROR;
            response->error  = ErrorCode::BAD_REQUEST;
            response->add_data.emplace("reason", std::string("missing local_path for PUT"));
            emit signalRecvMessage(response);
            return;
        }
        // We guarantee the destination directory.
        QFileInfo dst(bucket_path);
        QDir().mkpath(dst.absolutePath());
        // Overwrite if the file already exists.
        if (QFile::exists(bucket_path)) {
            QFile::remove(bucket_path);
        }
        if (!QFile::copy(local_path_in, bucket_path)) {
            response->status = MessageStatus::ERROR;
            response->error  = ErrorCode::INTERNAL_ERROR;
            response->add_data.emplace("reason", std::string("copy failed"));
            emit signalRecvMessage(response);
            return;
        }
        response->error = ErrorCode::SUCCESS;
        // For PUT we return only object_key/presigned_url - local
        // The FileManager bucket_path itself is not needed.
        emit signalRecvMessage(response);
        return;
    }

    // GET: if there is no file - ERROR, otherwise we give the path.
    if (!QFile::exists(bucket_path)) {
        response->status = MessageStatus::ERROR;
        response->error  = ErrorCode::SERVICE_UNAVAILABLE;
        response->add_data.emplace("reason", std::string("file not found in local bucket"));
        emit signalRecvMessage(response);
        return;
    }
    response->error = ErrorCode::SUCCESS;
    response->add_data.emplace("local_path", bucket_path.toStdString());
    emit signalRecvMessage(response);
}

void MinIOConnector::onShutdown()
{
    qDebug() << "MinIOConnector::onShutdown()";
    m_presignedByKey.clear();
}

} // namespace uniter::net
