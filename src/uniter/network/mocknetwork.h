#ifndef MOCKNETWORK_H
#define MOCKNETWORK_H

#include <QObject>
#include <memory>
#include <queue>
#include "../messages/unitermessage.h"

namespace uniter::net {


class MockNetManager : public QObject
{
    Q_OBJECT

public:
    explicit MockNetManager(QObject* parent = nullptr);
    ~MockNetManager() override;

    // === Управление соединением ===

    /// Установить соединение (имитирует подключение к серверу)
    void makeConnection();

    /// Потерять соединение (имитирует разрыв связи)
    void loseConnection();

    /// Проверить, установлено ли соединение
    bool isConnected() const;

    // === Управление сообщениями ===

    /// Отправить сообщение на "сервер" (добавить в очередь отправки)
    void sendMessage(std::shared_ptr<messages::UniterMessage> message);

    /// Имитировать получение сообщения с "сервера"
    void receiveMessage(std::shared_ptr<messages::UniterMessage> message);

    /// Получить размер очереди отправки
    int queueSize() const;

    // === Параметры для тестирования ===

    /// Установить задержку при отправке (мс)
    void setLatency(int milliseconds);

    /// Имитировать потерю пакета (вероятность 0.0-1.0)
    void setPacketLossRate(double rate);

public slots:
    /// Слот для получения сигналов от AppManager
    void onMakeConnection();
    void onSendMessage(std::shared_ptr<messages::UniterMessage> message);
    void onShutdown();

signals:
    /// Сигналы для AppManager и остального приложения

    /// Соединение установлено
    void signalConnected();
    void signalDisconnected();

    /// Сообщение получено с "сервера"
    void signalRecvMessage(std::shared_ptr<messages::UniterMessage> message);

    /// Сообщение отправлено на "сервер"
    void signalMessageSent(std::shared_ptr<messages::UniterMessage> message);

private:
    // === Состояние ===
    bool connected_ = false;

    // === Очередь сообщений ===
    std::queue<std::shared_ptr<messages::UniterMessage>> send_queue_;

    // === Параметры тестирования ===
    int latency_ms_ = 0;              // Задержка отправки
    double packet_loss_rate_ = 0.0;   // Вероятность потери пакета

    // === Счетчики для логирования ===
    int seq_id_sent_ = 0;
    int seq_id_received_ = 0;
};

} // namespace uniter::network

#endif // MOCKNETWORK_H
