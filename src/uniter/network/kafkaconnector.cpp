#include "kafkaconnector.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>

namespace uniter::net {

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
    // Успех подтверждаем сразу, чтобы AppManager/UI могли считать нас подписанными.
    QTimer::singleShot(0, this, [this]() {
        emit signalKafkaSubscribed(true);
    });
}

void KafkaConnector::onShutdown()
{
    qDebug() << "KafkaConnector::onShutdown()";
    m_initialized = false;
    m_userhash.clear();
}

} // namespace uniter::net
