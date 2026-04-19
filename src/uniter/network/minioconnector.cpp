#include "minioconnector.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>

namespace uniter::net {

MinIOConnector* MinIOConnector::instance()
{
    static MinIOConnector instance;
    return &instance;
}

MinIOConnector::MinIOConnector()
    : QObject(nullptr)
{
}

void MinIOConnector::onInitConnection()
{
    qDebug() << "MinIOConnector::onInitConnection() — stub init";

    m_initialized = true;

    // Произвольный offset, имитирующий значение из OS Secure Storage.
    const quint32 raw = QRandomGenerator::global()->bounded(100000u, 999999u);
    const QString offset = QStringLiteral("offset-%1").arg(raw);

    // Отвечаем асинхронно через event loop — это ближе к реальному поведению
    // (AppManager не должен получать сигнал внутри того же стека вызова слота).
    QTimer::singleShot(0, this, [this, offset]() {
        qDebug() << "MinIOConnector: emitting signalOffsetReady(" << offset << ")";
        emit signalOffsetReady(offset);
    });
}

void MinIOConnector::onSubscribeKafka()
{
    qDebug() << "MinIOConnector::onSubscribeKafka() — stub subscribe";
    // Успех подтверждаем сразу, чтобы AppManager/UI могли считать нас подписанными.
    QTimer::singleShot(0, this, [this]() {
        emit signalKafkaSubscribed(true);
    });
}

void MinIOConnector::onShutdown()
{
    qDebug() << "MinIOConnector::onShutdown()";
    m_initialized = false;
}

} // namespace uniter::net
