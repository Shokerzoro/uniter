#include "subsystemicon.h"
#include "../control/uimanager.h"
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QLabel>
#include <QVBoxLayout>

namespace uniter::genwdg {

SubsystemIcon::SubsystemIcon(contract::Subsystem subsystem,
                             contract::GenSubsystemType genType,
                             uint64_t genId,
                             int index,
                             QWidget* parent)
    : QWidget(parent), index(index) {

    QString name;
    if (subsystem == contract::Subsystem::GENERATIVE) {
        switch (genType) {
        case contract::GenSubsystemType::INTERGATION:
            name = "IGR";
            break;
        case contract::GenSubsystemType::PRODUCTION:
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

    // Создаем QLabel с текстом
    nameLabel = new QLabel(name, this);
    nameLabel->setAlignment(Qt::AlignCenter);

    // Создаем layout для центрирования
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(nameLabel);
    setLayout(layout);

    // Применяем настройки UIManager
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

    // Получаем constraints
    auto settings = control::UIManager::instance();
    const auto& constr = settings->getSubsIconConstraints();

    // Рисуем фон (дефолтный или hover)
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
