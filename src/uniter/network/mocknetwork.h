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

public slots:
    // Команда от AppManager: попытаться установить соединение
    void onMakeConnection();

    // Команда от AppManager: отправить сообщение на «сервер»
    void onSendMessage(std::shared_ptr<messages::UniterMessage> message);

    // Завершение работы (если нужно дергать явно)
    void onShutdown();

signals:
    // Network → AppManager
    void signalConnected();
    void signalDisconnected();

    // Network → AppManager: входящее сообщение
    void signalRecvMessage(std::shared_ptr<messages::UniterMessage> message);

private:
    bool connected_ = false;

    // Для статистики/отладки (при желании можно убрать)
    int seq_id_sent_ = 0;
    int seq_id_received_ = 0;
};

} // namespace uniter::net

#endif // MOCKNETWORK_H
