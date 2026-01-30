
#include "configmanager.h"
#include "../messages/unitermessage.h"
#include "../resources/employee/employee.h"
#include <QObject>
#include <QByteArray>
#include <optional>
#include <memory>


namespace uniter::managers {

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
    std::shared_ptr<messages::UniterMessage>            AuthMessage;
    std::shared_ptr<resources::employees::Employee>     User;

    // Менеджер конфигураций
    std::unique_ptr<ConfigManager> ConfigMgr;

public:
    AppManager();
    ~AppManager() override = default;

    void start_run();

public slots:
    // От сетевого класса
    void onConnected();
    void onDisconnected();

    // От менеджеров / переходы между состояниями
    void onResourcesLoaded();   // AUTHENTICATED -> DBLOADED (от БД после инициализации)
    void onConfigured();        // DBLOADED -> READY (от ConfigManager)
    void onShutDown();

    // От ConfigManager
    void onSubsystemAdded(messages::Subsystem subsystem,
                          messages::GenSubsystemType genType,
                          std::optional<uint64_t> genId,
                          bool created);

    // Маршрутизация сообщений
    void onRecvUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
    void onSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);

signals:
    // Внешние (к сетевому классу)
    void signalMakeConnection();
    void signalPollMessages();

    // Внутренние для UI
    void signalConnected();
    void signalDisconnected();
    void signalAuthed(bool result);

    // Внутренние для менеджеров
    void signalFindAuthData();
    void signalLoadResources(QByteArray userhash);  // Инициализация БД (AUTHENTICATED -> DBLOADED)
    void signalConfigProc(std::shared_ptr<resources::employees::Employee> User);  // Вызов ConfigManager (DBLOADED -> READY)

    // Для маршрутизации
    void signalRecvUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);

    // Передача информации о подсистемах вверх по стеку (при необходимости)
    void signalSubsystemAdded(messages::Subsystem subsystem,
                              messages::GenSubsystemType genType,
                              std::optional<uint64_t> genId,
                              bool created);
};

} // namespace uniter::managers


