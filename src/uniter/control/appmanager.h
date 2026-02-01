
#include "../contract/unitermessage.h"
#include "../contract/employee/employee.h"
#include <QObject>
#include <QByteArray>
#include <optional>
#include <memory>


namespace uniter::control {

class AppManager : public QObject {
    Q_OBJECT
protected:
    enum class AppState {
        IDLE,
        STARTED,
        CONNECTED,
        AUTHENTIFICATED,
        DBLOADED,      // Переименовано: DB_LOADED
        READY,
        SHUTDOWN
    };
    enum class NetState {ONLINE, OFFLINE};

    AppState AState = AppState::IDLE;
    NetState NState = NetState::OFFLINE;

    void SetAppState(AppState NewState);
    void SetNetState(NetState NewState);

    // Временное хранение данных аутентификации
    std::shared_ptr<contract::UniterMessage>            AuthMessage;
    std::shared_ptr<contract::employees::Employee>     User;
public:
    AppManager();
    ~AppManager() = default;
    // Удалям для безопасности
    AppManager(const AppManager&) = delete;
    AppManager& operator=(const AppManager&) = delete;
    AppManager(AppManager&&) = delete;
    AppManager& operator=(AppManager&&) = delete;

    // Запуск FSM
    void start_run();
public slots:
    // От сетевого класса
    void onConnectionUpdated(bool state);

    // От менеджеров / переходы между состояниями
    void onResourcesLoaded();   // AUTHENTICATED -> DBLOADED (от БД после инициализации)
    void onConfigured();        // DBLOADED -> READY (от ConfigManager)
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
    void signalLoadResources(QByteArray userhash);  // Инициализация БД (AUTHENTICATED -> DBLOADED)
    void signalConfigProc(std::shared_ptr<contract::employees::Employee> User);  // Вызов ConfigManager (DBLOADED -> READY)

    // Для маршрутизации
    void signalRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
};

} // namespace uniter::control


