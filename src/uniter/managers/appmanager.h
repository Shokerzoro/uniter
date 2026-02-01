
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
};

} // namespace uniter::managers


