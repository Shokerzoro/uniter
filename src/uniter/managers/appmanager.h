
#include "../messages/unitermessage.h"
#include "../resources/employee/employee.h"
#include <QScopedPointer>
#include <optional>

namespace uniter::managers {

// управляет приложением, хранит данные сессии
// выполняет типовые действия при старте/закрытии
class AppManager : public QObject {
    QOBJECT
private:
    enum class AppState {IDLE, STARTED, CONNECTED, AUTHENTIFICATED, CONFIGURATED, READY, SHUTDOWN};
    enum class NetState {ONLINE, OFFLINE};
    AppState AState = AppState::IDLE;
    NetState NState = NetState::OFFLINE;
    void SetAppState(AppState NewState);
    void SetNetState(NetState NewState);

    // Для временного хранения данных аутентификации
    // Если они пришли до signalFindAuthData()
    QScopedPointer<messages::UniterMessage> AuthMessage;
    // Данные пользователя и сессии
    std::optional<resources::employees::Employee> User = std::nullopt;
public:
    AppManager();
    ~AppManager();
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
    void onRecvUniterMessage(QScopedPointer<messages::UniterMessage> AuthMessage);
    void onSendUniterMeassage(QScopedPointer<messages::UniterMessage> AuthMessage);
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
    void signalConfigProc(resources::employees::Employee User);
    // Для применения локальных настроек (нахождение существующих проектов и т.д.)
    // После этого UniterMessage сможет передавать им данные (относится к проектам в первую очередь)
    void signalCustomizeProc();

    // Для маршрутизации
    void signalRecvUniterMessage(QScopedPointer<messages::UniterMessage> AuthMessage);
    void signalSendUniterMessage(QScopedPointer<messages::UniterMessage> AuthMessage);
};



} // managers
