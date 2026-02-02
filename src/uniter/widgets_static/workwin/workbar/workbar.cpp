
#include "../../../control/uimanager.h"
#include "../../../contract/unitermessage.h"
#include "../../../widgets_generative/subsystemicon.h"
#include "workbar.h"
#include <QWidget>
#include <QBoxLayout>
#include <QPainter>
#include <QString>

namespace uniter::staticwdg {


WorkBar::WorkBar(QWidget* parent) : QWidget(parent) {

    // Применяем UI настройки
    auto settings = control::UIManager::instance();
    settings->applyWorkBarSettings(this);

    // Создаём контейнер для иконок
    iconContainer = new QWidget(this);
    iconLayout = new QVBoxLayout(iconContainer);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->setSpacing(8);
    iconLayout->addStretch();  // Выталкиваем иконки вверх
    iconContainer->setLayout(iconLayout);

    // Основной layout WorkBar
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(iconContainer);
    setLayout(mainLayout);
}

void WorkBar::addSubsystem(contract::Subsystem subsystem,
                           contract::GenSubsystemType genType,
                           uint64_t genId,
                           int index) {

    qDebug() << "WorkBar::addSubsystem():" << subsystem;

    // Создаём новую иконку
    auto* icon = new genwdg::SubsystemIcon(subsystem, genType, genId, index);

    // Сохраняем в map
    subsystemIcons[index] = icon;

    // Добавляем в layout перед stretch'ем
    iconLayout->insertWidget(iconLayout->count() - 1, icon);

    // Коннектим сигнал иконки к слоту WorkBar
    connect(icon, &genwdg::SubsystemIcon::clicked,
            this, &WorkBar::onIconClicked);
}

void WorkBar::removeSubsystem(int index) {
    auto it = subsystemIcons.find(index);
    if (it != subsystemIcons.end()) {
        genwdg::SubsystemIcon* icon = it->second;

        // Отключаем сигнал
        disconnect(icon, &genwdg::SubsystemIcon::clicked,
                   this, &WorkBar::onIconClicked);

        // Удаляем из layout
        iconLayout->removeWidget(icon);

        // Удаляем виджет
        icon->deleteLater();

        // Удаляем из map
        subsystemIcons.erase(it);
    }
}

void WorkBar::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message) {
    emit signalSendUniterMessage(message);
}

void WorkBar::onIconClicked(int index) {
    emit signalSwitchTab(index);
}

void WorkBar::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Получаем constraints
    auto settings = control::UIManager::instance();
    const auto& constr = settings->getWorkBarConstraints();

    // Рисуем тонкую светлую полоску справа
    QPen pen(constr.borderColor);
    pen.setWidth(constr.borderWidth);
    painter.setPen(pen);

    // Линия от верха до низа по правому краю
    int x = width() - constr.borderWidth;
    painter.drawLine(x, 0, x, height());
}




} //uniter::staticwdg


