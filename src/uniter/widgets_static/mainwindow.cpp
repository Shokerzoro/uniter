
#include "../messages/unitermessage.h"
#include "./authwin/authwidget.h"
#include "./offlinewin/offlinewdg.h"
#include "./workwin/workwidget.h"
#include "mainwindow.h"
#include "uisettings.h"
#include <QStackedLayout>
#include <QWidget>

namespace uniter::staticwdg {

MainWidget::MainWidget(QWidget* parent) : QWidget(parent) {

    // Применяем настройки
    auto settings = UISettings::instance();
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

    connect(this, &MainWidget::signalSubsystemAdded,
            WWdg, &WorkWdg::onSubsystemAdded);

    connect(WWdg, &WorkWdg::signalSendUniterMessage,
            this, &MainWidget::onSendUniterMessage);

    // Начальное состояние
    MLayout->setCurrentWidget(AWdg);
}

void MainWidget::SetAuthState(AuthState newAState) {
    AState = newAState;

    if (newAState == AuthState::AUTHED) {
        MLayout->setCurrentWidget(WWdg);
    } else if (newAState == AuthState::NONE) {
        MLayout->setCurrentWidget(AWdg);
    }
}
void MainWidget::SetNetState(NetState newNState) {
    NState = newNState;
    if (newNState == NetState::OFFLINE) {
        MLayout->setCurrentWidget(OffWdg);
    } else if (newNState == NetState::ONLINE) {
        // Возвращаемся в виджет согласно AuthState
        if (AState == AuthState::AUTHED) {
            MLayout->setCurrentWidget(WWdg);
        } else if (AState == AuthState::NONE) {
            MLayout->setCurrentWidget(AWdg);
        }
    }
}


void MainWidget::onConnected() {
    SetNetState(NetState::ONLINE);
}

void MainWidget::onDisconnected() {
    SetNetState(NetState::OFFLINE);
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
    emit signalFindAuthData();
}

void MainWidget::onMakeConnect() {
    emit signalMakeConnect();
}

void MainWidget::onSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message) {
    emit signalSendUniterMessage(Message);
}

void MainWidget::onSubsystemAdded(messages::Subsystem subsystem,
                                  messages::GenSubsystemType genType,
                                  uint64_t genId,
                                  bool created) {
    emit signalSubsystemAdded(subsystem, genType, genId, created);
}

} // uniter::staticwdg
