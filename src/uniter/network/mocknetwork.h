
#ifndef MOCKNETWORK_H
#define MOCKNETWORK_H

#include <QObject>
#include <memory>
#include <queue>
#include "../contract/unitermessage.h"

namespace uniter::net {

class MockNetManager : public QObject
{
    Q_OBJECT

private:
    // Приватный конструктор для синглтона
    MockNetManager();

    // Запрет копирования и перемещения
    MockNetManager(const MockNetManager&) = delete;
    MockNetManager& operator=(const MockNetManager&) = delete;
    MockNetManager(MockNetManager&&) = delete;
    MockNetManager& operator=(MockNetManager&&) = delete;

    bool connected_ = false;

    // Для статистики/отладки (при желании можно убрать)
    int seq_id_sent_ = 0;
    int seq_id_received_ = 0;

public:
    // Публичный статический метод получения экземпляра
    static MockNetManager* instance();

    ~MockNetManager() override;

public slots:
    // Команда от AppManager: попытаться установить соединение
    void onMakeConnection();

    // Команда от AppManager: отправить сообщение на «сервер»
    void onSendMessage(std::shared_ptr<contract::UniterMessage> message);

    // Завершение работы (если нужно дергать явно)
    void onShutdown();

signals:
    // Network → AppManager
    void signalConnectionUpdated(bool state);

    // Network → AppManager: входящее сообщение
    void signalRecvMessage(std::shared_ptr<contract::UniterMessage> message);
};

} // namespace uniter::net

#endif // MOCKNETWORK_H
