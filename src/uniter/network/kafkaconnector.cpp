#include "kafkaconnector.h"
#include "../contract_qt/qt_compat.h"

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

    // An arbitrary offset that simulates the value from OS Secure Storage.
    // In real implementation: pull offset by key "kafka.offset.<userhash>".
    const quint32 raw = QRandomGenerator::global()->bounded(100000u, 999999u);
    const QString offset = QStringLiteral("offset-%1").arg(raw);

    // We respond asynchronously through the event loop - this is closer to real behavior
    // (AppManager should not receive a signal within the same slot call stack).
    QTimer::singleShot(0, this, [this, offset]() {
        qDebug() << "KafkaConnector: emitting signalOffsetReady(" << offset << ")";
        emit signalOffsetReady(offset);
    });
}

void KafkaConnector::onSubscribeKafka()
{
    qDebug() << "KafkaConnector::onSubscribeKafka() — stub subscribe";
    m_subscribed = true;
    // We confirm success immediately so that AppManager/UI can consider us signed up.
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

    // Broadcast queue invariant: only CRUD with NOTIFICATION.
    if (message->crudact == CrudAction::NOTCRUD ||
        message->status  != MessageStatus::NOTIFICATION)
    {
        qDebug() << "KafkaConnector::onServerNotification() — dropped, wrong type"
                 << "crudact=" << message->crudact
                 << "status="  << message->status;
        return;
    }

    // Before subscribing to a topic, there are no broadcast messages.
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
