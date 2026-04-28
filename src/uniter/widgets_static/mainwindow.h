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
        // Widget states
        enum class WidgetState {
            AUTH_OFFLINE,    // OfflineWdg (no network, not authorized)
            AUTH_ONLINE,     // AuthWdg (there is a network, not authorized)
            WORK_ONLINE,     // WorkWdg (there is a network, authorized)
            WORK_OFFLINE     // OfflineWdg (no network, but was authorized)
        };

        // Events for FSM
        enum class WidgetEvents {
            NET_CONNECTED,
            NET_DISCONNECTED,
            AUTH_SUCCESS,
            AUTH_FAILED,
            LOGOUT
        };

        WidgetState m_widgetState = WidgetState::AUTH_OFFLINE;

        // FSM event processing
        void ProcessEvent(WidgetEvents event);

        // Widgets
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
        // From the application manager
        void onConnectionUpdated(bool state);
        void onAuthed(bool result);
        void onFindAuthData();
        void onLoggedOut();
    signals:
        // For the authentication widget
        void signalAuthed(bool result);
        void signalFindAuthData();
};

} // namespace uniter::staticwdg


#endif // MAINWINDOW_H
