#include "subsystemicon.h"
#include "../managers/uimanager.h"
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QLabel>
#include <QVBoxLayout>

namespace uniter::genwdg {

SubsystemIcon::SubsystemIcon(messages::Subsystem subsystem,
                             messages::GenSubsystemType genType,
                             uint64_t genId,
                             int index,
                             QWidget* parent)
    : QWidget(parent), index(index) {

    QString name;
    if (subsystem == messages::Subsystem::GENERATIVE) {
        switch (genType) {
        case messages::GenSubsystemType::INTERGATION:
            name = "IGR";
            break;
        case messages::GenSubsystemType::PRODUCTION:
            name = "PRD";
            break;
        default:
            break;
        }
    } else {
        switch (subsystem) {
        case messages::Subsystem::DESIGN:
            name = "DGN";
            break;
        case messages::Subsystem::MANAGER:
            name = "MGR";
            break;
        case messages::Subsystem::MATERIALS:
            name = "MTR";
            break;
        case messages::Subsystem::PURCHASES:
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
    auto settings = managers::UIManager::instance();
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
    auto settings = managers::UIManager::instance();
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
