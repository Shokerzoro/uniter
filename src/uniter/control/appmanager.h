
#include "../contract/unitermessage.h"
#include "../contract/employee/employee.h"
#include <QObject>
#include <QByteArray>
#include <optional>
#include <memory>

namespace uniter::control {

class AppManager : public QObject {
    Q_OBJECT

public:
    // События для FSM
    enum class Events {
        START,              // Запуск приложения
        NET_CONNECTED,      // Сеть подключена
        NET_DISCONNECTED,   // Сеть отключена
        AUTH_SUCCESS,       // Успешная аутентификация
        DB_LOADED,          // БД загружена
        CONFIG_DONE,        // Конфигурация завершена
        SHUTDOWN            // Завершение работы
    };

private:
    // Состояния приложения
    enum class AppState {
        IDLE,
        STARTED,
        CONNECTED,
        AUTHENTIFICATED,
        DBLOADED,
        READY,
        SHUTDOWN
    };

    // Состояние сети (независимое от AppState)
    enum class NetState {
        ONLINE,
        OFFLINE
    };

    // Приватный конструктор для синглтона
    AppManager();

    // Запрет копирования и перемещения
    AppManager(const AppManager&) = delete;
    AppManager& operator=(const AppManager&) = delete;
    AppManager(AppManager&&) = delete;
    AppManager& operator=(AppManager&&) = delete;

    // Текущие состояния
    AppState m_appState = AppState::IDLE;
    NetState m_netState = NetState::OFFLINE;

    // Временное хранение данных
    std::shared_ptr<contract::UniterMessage> m_authMessage;
    std::shared_ptr<contract::employees::Employee> m_user;

    // Основной метод перехода между состояниями
    void ProcessEvent(Events event);

public:
    // Публичный статический метод получения экземпляра
    static AppManager* instance();

    ~AppManager() override = default;

    void start_run();

public slots:
    // От сетевого класса
    void onConnectionUpdated(bool state);

    // От менеджеров / переходы между состояниями
    void onResourcesLoaded();   // AUTHENTICATED -> DBLOADED
    void onConfigured();        // DBLOADED -> READY
    void onShutDown();

    // Маршрутизация сообщений
    void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);

signals:
    // Внешние (к сетевому классу)
    void signalMakeConnection();
    void signalPollMessages();

    // Внутренние для UI
    void signalConnectionUpdated(bool state);
    void signalAuthed(bool result);

    // Внутренние для менеджеров
    void signalFindAuthData();
    void signalLoadResources(QByteArray userhash);
    void signalConfigProc(std::shared_ptr<contract::employees::Employee> User);
    // И для очистки ресурсов
    void signalClearResources();

    // Для маршрутизации
    void signalRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
};


} // namespace uniter::control


