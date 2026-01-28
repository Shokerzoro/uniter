#include "subsystemicon.h"
#include <QBoxLayout>
#include <QWidget>
#include <QString>

namespace uniter::genwdg {

SubsystemIcon::SubsystemIcon(int index, QWidget* parent)
    : QWidget(parent), index(index) {

    auto* layout = new QVBoxLayout(this);
    nameLabel = new QLabel(this);
    nameLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(nameLabel);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

void SubsystemIcon::mousePressEvent(QMouseEvent* event) {
    emit clicked(index);
    QWidget::mousePressEvent(event);
}

void SubsystemIcon::setName(const QString& name_) {
    nameLabel->setText(name_);
}

} // genwdg
