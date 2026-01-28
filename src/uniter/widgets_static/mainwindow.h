    #ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../messages/unitermessage.h"
#include "../resources/resourceabstract.h"
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
    void onSubsystemAdded(messages::Subsystem subsystem,
                          messages::GenSubsystemType genType,
                          uint64_t genId,
                          bool created);
    // От нижнего уровня
    void onMakeConnect();
    void onSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);

signals:
    // Для виджета аутентификации
    void signalAuthed(bool result);
    void signalFindAuthData();
    // От виджета оффлайна
    void signalMakeConnect();
    // Для рабочего виджета
    void signalSubsystemAdded(messages::Subsystem subsystem,
                              messages::GenSubsystemType genType,
                              uint64_t genId,
                              bool created);
    // Для маршрутизации сообщений
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);

    // Подписка на ресурсы
    void signalSubscribeToResourceList(messages::Subsystem subsystem, messages::ResourceType type, std::shared_ptr<genwdg::ISubsWdg> observer);
    void signalSubscribeToResourceTree(messages::Subsystem subsystem, messages::ResourceType type, std::shared_ptr<genwdg::ISubsWdg> observer);
    void signalSubscribeToResource(messages::Subsystem subsystem, messages::ResourceType type, uint64_t resId, std::shared_ptr<genwdg::ISubsWdg> observer);
    // Получить ресурс
    void signalGetResource(messages::Subsystem subsystem, messages::ResourceType type, uint64_t resId, std::shared_ptr<genwdg::ISubsWdg> observer);
};

} // namespace uniter::staticwdg


#endif // MAINWINDOW_H
