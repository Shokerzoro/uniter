
#include "../contract/unitermessage.h"
#include "../control/uimanager.h"
#include "./authwin/authwidget.h"
#include "./offlinewin/offlinewdg.h"
#include "./workwin/workwidget.h"
#include "mainwindow.h"
#include <QStackedLayout>
#include <QWidget>
#include <QDebug>

    namespace uniter::staticwdg {

    MainWidget::MainWidget(QWidget* parent) : QWidget(parent) {

        // Применяем настройки
        auto settings = control::UIManager::instance();
        settings->applyMainWinSettings(this);

        // Создаем stacked layout
        MLayout = new QStackedLayout(this);
        setLayout(MLayout);

        // Виджет аутентификации
        AWdg = new AuthWdg();
        MLayout->addWidget(AWdg);

        QObject::connect(this, &MainWidget::signalAuthed,
                         AWdg, &AuthWdg::onAuthed);

        QObject::connect(this, &MainWidget::signalFindAuthData,
                         AWdg, &AuthWdg::onFindAuthData);

        QObject::connect(AWdg, &AuthWdg::signalSendUniterMessage,
                         this, &MainWidget::onSendUniterMessage);

        // Виджет оффлайна
        OffWdg = new OfflineWdg();
        MLayout->addWidget(OffWdg);

        connect(OffWdg, &OfflineWdg::signalMakeConnect,
                this,   &MainWidget::onMakeConnect);

        // Рабочий виджет
        WWdg = new WorkWdg();
        MLayout->addWidget(WWdg);

        connect(WWdg, &WorkWdg::signalSendUniterMessage,
                this, &MainWidget::onSendUniterMessage);

        // Начальное состояние
        MLayout->setCurrentWidget(OffWdg);
    }

    // ========== FSM ОБРАБОТКА СОБЫТИЙ ==========

    void MainWidget::ProcessEvent(WidgetEvents event)
    {
        qDebug() << "MainWidget::ProcessEvent() - Event received";

        // Лямбды только для входа в состояние
        auto in_auth_offline = [this]() {
            qDebug() << "MainWidget::in_auth_offline()";
            MLayout->setCurrentWidget(OffWdg);
        };

        auto in_auth_online = [this]() {
            qDebug() << "MainWidget::in_auth_online()";
            MLayout->setCurrentWidget(AWdg);
        };

        auto in_work_online = [this]() {
            qDebug() << "MainWidget::in_work_online()";
            MLayout->setCurrentWidget(WWdg);
            emit signalAuthed(true);
        };

        auto in_work_offline = [this]() {
            qDebug() << "MainWidget::in_work_offline()";
            MLayout->setCurrentWidget(OffWdg);
        };

        // Обработка событий в зависимости от текущего состояния
        switch (m_widgetState) {
        case WidgetState::AUTH_OFFLINE:
            if (event == WidgetEvents::NET_CONNECTED) {
                m_widgetState = WidgetState::AUTH_ONLINE;
                in_auth_online();
            }
            break;

        case WidgetState::AUTH_ONLINE:
            if (event == WidgetEvents::AUTH_SUCCESS) {
                m_widgetState = WidgetState::WORK_ONLINE;
                in_work_online();
            }
            else if (event == WidgetEvents::AUTH_FAILED) {
                qDebug() << "MainWidget::ProcessEvent(): AUTH_FAILED - staying in AUTH_ONLINE";
                emit signalAuthed(false);
            }
            else if (event == WidgetEvents::NET_DISCONNECTED) {
                m_widgetState = WidgetState::AUTH_OFFLINE;
                in_auth_offline();
            }
            break;

        case WidgetState::WORK_ONLINE:
            if (event == WidgetEvents::NET_DISCONNECTED) {
                m_widgetState = WidgetState::WORK_OFFLINE;
                in_work_offline();
            }
            else if (event == WidgetEvents::LOGOUT) {
                m_widgetState = WidgetState::AUTH_ONLINE;
                in_auth_online();
            }
            break;

        case WidgetState::WORK_OFFLINE:
            if (event == WidgetEvents::NET_CONNECTED) {
                m_widgetState = WidgetState::WORK_ONLINE;
                in_work_online();
            }
            else if (event == WidgetEvents::LOGOUT) {
                m_widgetState = WidgetState::AUTH_OFFLINE;
                in_auth_offline();
            }
            break;
        }
    }

    // ========== СЛОТЫ ==========

    void MainWidget::onConnectionUpdated(bool state)
    {
        qDebug() << "MainWidget::onConnectionUpdated(" << state << ")";

        if (state) {
            ProcessEvent(WidgetEvents::NET_CONNECTED);
        } else {
            ProcessEvent(WidgetEvents::NET_DISCONNECTED);
        }
    }

    void MainWidget::onAuthed(bool result)
    {
        qDebug() << "MainWidget::onAuthed(" << result << ")";

        if (result) {
            ProcessEvent(WidgetEvents::AUTH_SUCCESS);
        } else {
            ProcessEvent(WidgetEvents::AUTH_FAILED);
        }
    }

    void MainWidget::onFindAuthData()
    {
        qDebug() << "MainWidget::onFindAuthData()";
        emit signalFindAuthData();
    }

    void MainWidget::onLoggedOut()
    {
        qDebug() << "MainWidget::onLoggedOut()";
        ProcessEvent(WidgetEvents::LOGOUT);
    }

    void MainWidget::onMakeConnect()
    {
        qDebug() << "MainWidget::onMakeConnect()";
        emit signalMakeConnect();
    }

    void MainWidget::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message)
    {
        emit signalSendUniterMessage(Message);
    }

} // namespace uniter::staticwdg

