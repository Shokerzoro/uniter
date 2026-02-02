    #ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../contract/unitermessage.h"
#include "../contract/resourceabstract.h"
#include "./authwin/authwidget.h"
#include "./offlinewin/offlinewdg.h"
#include "./workwin/workwidget.h"
#include <QStackedLayout>
#include <QWidget>

namespace uniter::staticwdg {

class MainWidget : public QWidget {
    Q_OBJECT

private:
    // Классы состояний
    enum class AuthState {NONE, AUTHED, READY};
    enum class NetState {OFFLINE, ONLINE};
    AuthState AState = AuthState::NONE;
    NetState NState = NetState::OFFLINE;

    // Управление машиной состояний (реализуют логику управления при переходах)
    void SetAuthState(AuthState newAState);
    void SetNetState(NetState newNState);

    // Виджеты
    QStackedLayout* MLayout = nullptr;
    AuthWdg* AWdg = nullptr;
    OfflineWdg* OffWdg = nullptr;
    WorkWdg* WWdg = nullptr;

public:
    explicit MainWidget(QWidget* parent = nullptr);
    MainWidget(const MainWidget&) = delete;
    MainWidget(MainWidget&&) = delete;
    MainWidget& operator=(const MainWidget&) = delete;
    MainWidget& operator=(MainWidget&&) = delete;

public slots:
    // От менеджера приложения
    void onConnected();
    void onDisconnected();
    void onAuthed(bool result);
    void onFindAuthData();

    // От нижнего уровня
    void onMakeConnect();
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);

signals:
    // Для виджета аутентификации
    void signalAuthed(bool result);
    void signalFindAuthData();
    // От виджета оффлайна
    void signalMakeConnect();

    // Для маршрутизации сообщений
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);};

} // namespace uniter::staticwdg


#endif // MAINWINDOW_H
