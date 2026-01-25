
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

    // Начальное состояние
    MLayout->setCurrentWidget(AWdg);
}

void MainWidget::SetAuthState(AuthState newAState) {

}

void MainWidget::SetNetState(NetState newNState) {

}

void MainWidget::onConnected() {

}

void MainWidget::onDisconnected() {

}

void MainWidget::onAuthed(bool result) {

}

void MainWidget::onFindAuthData() {
    emit signalFindAuthData();
}

void MainWidget::onMakeConnect() {
    emit signalMakeConnect();
}

void MainWidget::onSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message) {

}

} // uniter::staticwdg
