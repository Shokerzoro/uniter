#include "kafkaconnector.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>

namespace uniter::net {

using namespace uniter::contract;

KafkaConnector* KafkaConnector::instance()
{
    static KafkaConnector instance;
    return &instance;
}

KafkaConnector::KafkaConnector()
    : QObject(nullptr)
{
}

void KafkaConnector::onInitConnection(QByteArray userhash)
{
    qDebug() << "KafkaConnector::onInitConnection() — stub init, userhash size="
             << userhash.size();

    m_userhash = std::move(userhash);
    m_initialized = true;
    m_subscribed  = false;

    // Произвольный offset, имитирующий значение из OS Secure Storage.
    // В реальной реализации: вытянуть offset по ключу "kafka.offset.<userhash>".
    const quint32 raw = QRandomGenerator::global()->bounded(100000u, 999999u);
    const QString offset = QStringLiteral("offset-%1").arg(raw);

    // Отвечаем асинхронно через event loop — это ближе к реальному поведению
    // (AppManager не должен получать сигнал внутри того же стека вызова слота).
    QTimer::singleShot(0, this, [this, offset]() {
        qDebug() << "KafkaConnector: emitting signalOffsetReady(" << offset << ")";
        emit signalOffsetReady(offset);
    });
}

void KafkaConnector::onSubscribeKafka()
{
    qDebug() << "KafkaConnector::onSubscribeKafka() — stub subscribe";
    m_subscribed = true;
    // Успех подтверждаем сразу, чтобы AppManager/UI могли считать нас подписанными.
    QTimer::singleShot(0, this, [this]() {
        emit signalKafkaSubscribed(true);
    });
}

void KafkaConnector::onShutdown()
{
    qDebug() << "KafkaConnector::onShutdown()";
    m_initialized = false;
    m_subscribed  = false;
    m_userhash.clear();
}

void KafkaConnector::onServerNotification(std::shared_ptr<contract::UniterMessage> message)
{
    if (!message) return;

    // Инвариант broadcast-очереди: только CRUD с NOTIFICATION.
    if (message->crudact == CrudAction::NOTCRUD ||
        message->status  != MessageStatus::NOTIFICATION)
    {
        qDebug() << "KafkaConnector::onServerNotification() — dropped, wrong type"
                 << "crudact=" << message->crudact
                 << "status="  << message->status;
        return;
    }

    // До подписки на топик broadcast-сообщений не существует.
    if (!m_subscribed) {
        qDebug() << "KafkaConnector::onServerNotification() — dropped, not subscribed yet";
        return;
    }

    qDebug() << "KafkaConnector: broadcasting NOTIFICATION upstream"
             << "subsystem=" << message->subsystem
             << "crudact="   << message->crudact;
    emit signalRecvMessage(message);
}

} // namespace uniter::net
