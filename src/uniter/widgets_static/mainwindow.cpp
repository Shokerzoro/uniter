
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
    MLayout->setCurrentWidget(AWdg);
}

void MainWidget::SetAuthState(AuthState newAState) {
    AState = newAState;

    if (newAState == AuthState::AUTHED) {
        qDebug() << "MainWidget::SetAuthState(): AUTHED";
        MLayout->setCurrentWidget(WWdg);
    } else if (newAState == AuthState::NONE) {
        qDebug() << "MainWidget::SetAuthState(): NONE";
        MLayout->setCurrentWidget(AWdg);
    }
}
void MainWidget::SetNetState(NetState newNState) {
    NState = newNState;
    if (newNState == NetState::OFFLINE) {
        qDebug() << "MainWidget::SetNetState(): OfflineWdg";
        MLayout->setCurrentWidget(OffWdg);
    } else if (newNState == NetState::ONLINE) {

        // Возвращаемся в виджет согласно AuthState
        if (AState == AuthState::AUTHED) {
            qDebug() << "MainWidget::SetNetState(): WorkWdg";
            MLayout->setCurrentWidget(WWdg);
        } else if (AState == AuthState::NONE) {
            qDebug() << "MainWidget::SetNetState(): AuthWidget";
            MLayout->setCurrentWidget(AWdg);
        }
    }
}

void MainWidget::onConnectionUpdated(bool state) {
    if (state) {
        qDebug() << "MainWidget::onConnected()";
        SetNetState(NetState::ONLINE);
    } else {
        qDebug() << "MainWidget::onDisconnected()";
        SetNetState(NetState::OFFLINE);
    }
}

void MainWidget::onAuthed(bool result) {
    if (!result) {
        emit signalAuthed(false);
    }
    else { SetAuthState(AuthState::AUTHED);
        emit signalAuthed(true);
    }
}

void MainWidget::onFindAuthData() {

    qDebug() << "MainWidget::onFindAuthData()";

    emit signalFindAuthData();
}

void MainWidget::onMakeConnect() {

    qDebug() << "MainWidget::onMakeConnect()";

    emit signalMakeConnect();
}

void MainWidget::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message) {
    emit signalSendUniterMessage(Message);
}



} // uniter::staticwdg
