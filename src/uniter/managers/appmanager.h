
#include "../messages/unitermessage.h"
#include "../resources/employee/employee.h"
#include <optional>
#include <memory>

namespace uniter::managers {

// управляет приложением, хранит данные сессии
// выполняет типовые действия при старте/закрытии
class AppManager : public QObject {
    Q_OBJECT
private:
    enum class AppState {IDLE, STARTED, CONNECTED, AUTHENTIFICATED, CONFIGURATED, READY, SHUTDOWN};
    enum class NetState {ONLINE, OFFLINE};
    AppState AState = AppState::IDLE;
    NetState NState = NetState::OFFLINE;
    void SetAppState(AppState NewState);
    void SetNetState(NetState NewState);

    // Для временного хранения данных аутентификации
    // Если они пришли до signalFindAuthData()
    std::shared_ptr<messages::UniterMessage> AuthMessage;
    std::shared_ptr<resources::employees::Employee> User;
public:
    AppManager() = default;
    ~AppManager() = default;
    void start_run();
public slots:
    // От сетевого класса
    void onConnected();
    void onDisconnected();

    // От менеджеров / выключение
    void onConfigured();
    void onCustomized();
    void onShutDown();

    // Маршрутизация сообщений
    void onRecvUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
    void onSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
signals:
    // Внешние
    void signalMakeConnection();
    void signalPollMessages();

    // Внутренние для UI
    void signalConnected(); // Разблокировка UI (кроме Connected, т.к. не заблокировано)
    void signalDisconnected(); // Блокировка UI
    void signalAuthed(bool result); // Для переключения UI в основной рабочий виджет

    // Внутренние
    void signalFindAuthData(); // Для виджета аутентификации
    void signalConfigProc(std::shared_ptr<resources::employees::Employee> User);
    // Для применения локальных настроек (нахождение существующих проектов и т.д.)
    // После этого UniterMessage сможет передавать им данные (относится к проектам в первую очередь)
    void signalCustomizeProc();

    // Для маршрутизации
    void signalRecvUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
};



} // managers
