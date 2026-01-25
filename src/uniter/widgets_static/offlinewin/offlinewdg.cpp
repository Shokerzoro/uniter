#include "offlinewdg.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace uniter::staticwdg {

OfflineWdg::OfflineWdg(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *label = new QLabel(tr("Соединение потеряно"), this);
    label->setAlignment(Qt::AlignCenter);

    auto *btn = new QPushButton(tr("Подключиться"), this);
    layout->addWidget(label);
    layout->addWidget(btn);
    layout->addStretch();

    connect(btn, &QPushButton::clicked, this, &OfflineWdg::signalMakeConnect);
}

} // namespace uniter::staticwdg

