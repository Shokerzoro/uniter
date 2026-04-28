
#include "../../../control/appmanager.h"
#include "../../../control/uimanager.h"
#include "../../../contract/unitermessage.h"
#include "../../../contract_qt/qt_compat.h"
#include "../../../widgets_generative/subsystemicon.h"
#include "workbar.h"
#include <QWidget>
#include <QBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QString>

namespace uniter::staticwdg {

WorkBar::WorkBar(QWidget* parent) : QWidget(parent) {

    // Applying UI settings
    auto settings = control::UIManager::instance();
    settings->applyWorkBarSettings(this);

    // Creating a container for icons
    iconContainer = new QWidget(this);
    iconLayout = new QVBoxLayout(iconContainer);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->setSpacing(8);
    iconLayout->addStretch();  // Push icons up
    iconContainer->setLayout(iconLayout);

    // Create a LogOut button
    logoutButton = new QPushButton("LogOut", this);
    logoutButton->setFixedSize(70, 70);

    // Connecting the button signal
    auto AManager = control::AppManager::instance();
    connect(logoutButton, &QPushButton::clicked,
            AManager, &control::AppManager::onLogout);

    // Main layout WorkBar
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(iconContainer);      // Subsystem icons
    mainLayout->addWidget(logoutButton);       // LogOut button at the bottom
    setLayout(mainLayout);
}

void WorkBar::addSubsystem(contract::Subsystem subsystem,
                           contract::GenSubsystem genType,
                           uint64_t genId,
                           int index) {

    qDebug() << "WorkBar::addSubsystem():" << subsystem;

    // Creating a new icon
    auto* icon = new genwdg::SubsystemIcon(subsystem, genType, genId, index);

    // Save to map
    subsystemIcons[index] = icon;

    // Add to layout before stretch
    iconLayout->insertWidget(iconLayout->count() - 1, icon);

    // Connect the icon signal to the WorkBar slot
    connect(icon, &genwdg::SubsystemIcon::clicked,
            this, &WorkBar::onIconClicked);
}

void WorkBar::removeSubsystem(int index) {
    auto it = subsystemIcons.find(index);
    if (it != subsystemIcons.end()) {
        genwdg::SubsystemIcon* icon = it->second;

        // Turn off the signal
        disconnect(icon, &genwdg::SubsystemIcon::clicked,
                   this, &WorkBar::onIconClicked);

        // Remove from layout
        iconLayout->removeWidget(icon);

        // Removing the widget
        icon->deleteLater();

        // Remove from map
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

    // We get constraints
    auto settings = control::UIManager::instance();
    const auto& constr = settings->getWorkBarConstraints();

    // Draw a thin light stripe on the right
    QPen pen(constr.borderColor);
    pen.setWidth(constr.borderWidth);
    painter.setPen(pen);

    // Line from top to bottom on the right edge
    int x = width() - constr.borderWidth;
    painter.drawLine(x, 0, x, height());
}

} //uniter::staticwdg
