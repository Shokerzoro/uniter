#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../messages/unitermessage.h"
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
    // Удаление конструкторов копирования и перемещения
    MainWidget(const MainWidget&) = delete;
    MainWidget(MainWidget&&) = delete;
    MainWidget& operator=(const MainWidget&) = delete;
    MainWidget& operator=(MainWidget&&) = delete;
public slots:
    // От менеджера приложения (триггерят управление машиной состояний)
    void onConnected();
    void onDisconnected();
    void onAuthed(bool result); // Передаем также виджету аутентификации
    void onFindAuthData();
    void onSubsystemAdded(messages::Subsystem subsystem, messages::GenSubsystemType genType, uint64_t genId, bool created);
    // От нижнего уровня
    void onMakeConnect();
    void onSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
signals:
    // Для виджета аутентификации (вниз)
    void signalAuthed(bool result);
    void signalFindAuthData();
    // От виджета оффлайна
    void signalMakeConnect(void);
    // Для рабочего виджета
    void signalSubsystemAdded(messages::Subsystem subsystem, messages::GenSubsystemType genType, uint64_t genId, bool created);
    // Для маршрутизации сообщений (вверх)
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
} ;


} // uniter::staticwdg


#endif // MAINWINDOW_H
