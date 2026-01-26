
#include "../../../messages/unitermessage.h"
#include "../../../widgets_generative/subsystemicon.h"
#include "statusbar.h"
#include <QString>

namespace uniter::staticwdg {



StatusBar::StatusBar(QWidget *parent) : QWidget{parent}
{
    // TODO: добавить контейнер для иконок

}


void StatusBar::addSubsystem(messages::GenSubsystemType genType,
                             const QString& name,
                             int index) {
    // Создаём новую иконку
    auto* icon = new genwdg::SubsystemIcon(index, this);

    // TODO: настройка иконки по genType и name
    // icon->setIcon(...);
    // icon->setToolTip(name);

    // Сохраняем в map
    subsystemIcons[index] = icon;

    // TODO: добавляем в layout StatusBar
    // layout->addWidget(icon);

    // Коннектим сигнал иконки к слоту StatusBar
    connect(icon, &genwdg::SubsystemIcon::clicked,
            this, &StatusBar::onIconClicked);
}

void StatusBar::removeSubsystem(int index) {
    auto it = subsystemIcons.find(index);
    if (it != subsystemIcons.end()) {
        genwdg::SubsystemIcon* icon = it->second;

        // Отключаем сигнал (опционально, т.к. удаляем виджет)
        disconnect(icon, &genwdg::SubsystemIcon::clicked,
                   this, &StatusBar::onIconClicked);

        // Удаляем виджет (удалит и из layout)
        icon->deleteLater();

        // Удаляем из map
        subsystemIcons.erase(it);
    }
}

void StatusBar::onSendUniterMessage(std::shared_ptr<messages::UniterMessage> message) {
    emit signalSendUniterMessage(message);
}

void StatusBar::onIconClicked(int index) {
    emit signalSwitchTab(index);
}



} //uniter::staticwdg


