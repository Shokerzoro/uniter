#include "mocknetwork.h"

#include <QDebug>
#include <cstdlib>
#include <ctime>


namespace uniter::net {


namespace uniter::network {

MockNetworkProcessor::MockNetworkProcessor(QObject* parent)
    : QObject(parent)
    , connected_(false)
    , latency_ms_(0)
    , packet_loss_rate_(0.0)
    , seq_id_sent_(0)
    , seq_id_received_(0)
{

}

MockNetworkProcessor::~MockNetworkProcessor()
{

}

void MockNetworkProcessor::makeConnection()
{
    if (!connected_) {
        connected_ = true;
        qDebug() << "Mock: Connection ESTABLISHED";
        emit signalConnectionUpdated(true);
    }
}

void MockNetworkProcessor::loseConnection()
{
    if (connected_) {
        connected_ = false;
        qDebug() << "Mock: Connection LOST";
        emit signalConnectionUpdated(false);
    }
}

bool MockNetworkProcessor::isConnected() const
{
    return connected_;
}

void MockNetworkProcessor::sendMessage(std::shared_ptr<messages::UniterMessage> message)
{
    if (!message) return;

    // Имитировать потерю пакета
    if (packet_loss_rate_ > 0.0) {
        double rand_val = static_cast<double>(std::rand()) / RAND_MAX;
        if (rand_val < packet_loss_rate_) {
            qDebug() << "Mock: Message LOST (packet loss simulation)";
            return;
        }
    }

    send_queue_.push(message);
    seq_id_sent_++;

    qDebug() << "Mock: Message queued for sending. Queue size:" << send_queue_.size();

    // Имитировать отправку после задержки
    if (connected_ && latency_ms_ == 0) {
        // Отправить сразу
        auto sent_msg = send_queue_.front();
        send_queue_.pop();
        qDebug() << "Mock: Message SENT (seq_id:" << seq_id_sent_ << ")";
        emit signalMessageSent(sent_msg);
    }
}

void MockNetworkProcessor::receiveMessage(std::shared_ptr<messages::UniterMessage> message)
{
    if (!message || !connected_) return;

    seq_id_received_++;
    qDebug() << "Mock: Message RECEIVED (seq_id:" << seq_id_received_ << ")";
    emit signalRecvMessage(message);
}

int MockNetworkProcessor::queueSize() const
{
    return send_queue_.size();
}

void MockNetworkProcessor::setLatency(int milliseconds)
{
    latency_ms_ = milliseconds;
    qDebug() << "Mock: Latency set to" << latency_ms_ << "ms";
}

void MockNetworkProcessor::setPacketLossRate(double rate)
{
    packet_loss_rate_ = rate;
    qDebug() << "Mock: Packet loss rate set to" << (rate * 100) << "%";
}

void MockNetworkProcessor::onMakeConnection()
{
    makeConnection();
}

void MockNetworkProcessor::onSendMessage(std::shared_ptr<messages::UniterMessage> message)
{
    sendMessage(message);
}

void MockNetworkProcessor::onShutdown()
{
    loseConnection();
    while (!send_queue_.empty()) {
        send_queue_.pop();
    }
    qDebug() << "Mock: Shutdown complete";
}

} // namespace uniter::network


} // net
