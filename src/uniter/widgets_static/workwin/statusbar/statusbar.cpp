
#include "../../../messages/unitermessage.h"
#include "../../../widgets_generative/subsystemicon.h"
#include "statusbar.h"
#include <QWidget>
#include <QBoxLayout>
#include <QString>

namespace uniter::staticwdg {


StatusBar::StatusBar(QWidget* parent) : QWidget(parent) {
    // Создаём контейнер для иконок
    iconContainer = new QWidget(this);
    iconLayout = new QVBoxLayout(iconContainer);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->setSpacing(8);
    iconLayout->addStretch();  // Выталкиваем иконки вверх
    iconContainer->setLayout(iconLayout);

    // Основной layout StatusBar
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(iconContainer);
    setLayout(mainLayout);
}

void StatusBar::addSubsystem(messages::GenSubsystemType genType,
                             const QString& name,
                             int index) {
    // Создаём новую иконку
    auto* icon = new genwdg::SubsystemIcon(index, this);
    icon->setName(name);

    // Сохраняем в map
    subsystemIcons[index] = icon;

    // Добавляем в layout перед stretch'ем
    iconLayout->insertWidget(iconLayout->count() - 1, icon);

    // Коннектим сигнал иконки к слоту StatusBar
    connect(icon, &genwdg::SubsystemIcon::clicked,
            this, &StatusBar::onIconClicked);
}

void StatusBar::removeSubsystem(int index) {
    auto it = subsystemIcons.find(index);
    if (it != subsystemIcons.end()) {
        genwdg::SubsystemIcon* icon = it->second;

        // Отключаем сигнал
        disconnect(icon, &genwdg::SubsystemIcon::clicked,
                   this, &StatusBar::onIconClicked);

        // Удаляем из layout
        iconLayout->removeWidget(icon);

        // Удаляем виджет
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


