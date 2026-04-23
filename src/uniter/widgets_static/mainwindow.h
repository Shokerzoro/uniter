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
        // Состояния виджета
        enum class WidgetState {
            AUTH_OFFLINE,    // OfflineWdg (нет сети, не авторизован)
            AUTH_ONLINE,     // AuthWdg (есть сеть, не авторизован)
            WORK_ONLINE,     // WorkWdg (есть сеть, авторизован)
            WORK_OFFLINE     // OfflineWdg (нет сети, но был авторизован)
        };

        // События для FSM
        enum class WidgetEvents {
            NET_CONNECTED,
            NET_DISCONNECTED,
            AUTH_SUCCESS,
            AUTH_FAILED,
            LOGOUT
        };

        WidgetState m_widgetState = WidgetState::AUTH_OFFLINE;

        // FSM обработка событий
        void ProcessEvent(WidgetEvents event);

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
        void onConnectionUpdated(bool state);
        void onAuthed(bool result);
        void onFindAuthData();
        void onLoggedOut();
    signals:
        // Для виджета аутентификации
        void signalAuthed(bool result);
        void signalFindAuthData();
};

} // namespace uniter::staticwdg


#endif // MAINWINDOW_H
