#include "subsystemicon.h"

namespace uniter::genwdg {

SubsystemIcon::SubsystemIcon(int index, QWidget* parent)
    : QWidget(parent), index(index) {
    // TODO: инициализация UI (иконка, стили), применение uisettings
}

void SubsystemIcon::mousePressEvent(QMouseEvent* event) {
    emit clicked(index);
    QWidget::mousePressEvent(event);
}

} // genwdg
