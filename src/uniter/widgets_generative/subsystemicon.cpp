#include "subsystemicon.h"
#include "../control/uimanager.h"
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QLabel>
#include <QVBoxLayout>

namespace uniter::genwdg {

SubsystemIcon::SubsystemIcon(contract::Subsystem subsystem,
                             contract::GenSubsystem genType,
                             uint64_t genId,
                             int index,
                             QWidget* parent)
    : QWidget(parent), index(index) {

    QString name;
    if (subsystem == contract::Subsystem::GENERATIVE) {
        switch (genType) {
        case contract::GenSubsystem::INTERGATION:
            name = "IGR";
            break;
        case contract::GenSubsystem::PRODUCTION:
            name = "PRD";
            break;
        default:
            break;
        }
    } else {
        switch (subsystem) {
        case contract::Subsystem::DESIGN:
            name = "DGN";
            break;
        case contract::Subsystem::MANAGER:
            name = "MGR";
            break;
        case contract::Subsystem::MATERIALS:
            name = "MTR";
            break;
        case contract::Subsystem::PURCHASES:
            name = "PRCH";
            break;
        default:
            break;
        }
    }

    // Create a QLabel with text
    nameLabel = new QLabel(name, this);
    nameLabel->setAlignment(Qt::AlignCenter);

    // Create a layout for centering
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(nameLabel);
    setLayout(layout);

    // Applying UIManager settings
    auto settings = control::UIManager::instance();
    settings->applySubsIconSettings(this);
}

void SubsystemIcon::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(index);
    }
    QWidget::mousePressEvent(event);
}

void SubsystemIcon::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // We get constraints
    auto settings = control::UIManager::instance();
    const auto& constr = settings->getSubsIconConstraints();

    // Draw the background (default or hover)
    if (isHovered) {
        painter.setBrush(constr.hoverBgColor);
    } else {
        painter.setBrush(constr.defaultBgColor);
    }

    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 8, 8);
}


void SubsystemIcon::enterEvent(QEnterEvent* event) {
    isHovered = true;
    update();
    QWidget::enterEvent(event);
}

void SubsystemIcon::leaveEvent(QEvent* event) {
    isHovered = false;
    update();
    QWidget::leaveEvent(event);
}

} // namespace uniter::genwdg
