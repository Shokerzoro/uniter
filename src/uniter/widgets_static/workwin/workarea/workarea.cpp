
#include "../../../messages/unitermessage.h"
#include "../../../widgets_generative/generativetab.h"
#include "workarea.h"
#include <QWidget>
#include <QStackedLayout>
#include <map>
#include <memory>
#include <cstdint>

namespace uniter::staticwdg {


WorkArea::WorkArea(QWidget* parent) : QWidget(parent) {
    stackedLayout = new QStackedLayout(this);
    stackedLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(stackedLayout);
}

void WorkArea::addSubsystem(messages::Subsystem subsystem,
                            messages::GenSubsystemType genType,
                            uint64_t genId,
                            int index) {

    qDebug() << "WorkArea::addSubsystem():" << subsystem;

    auto* subsWdg = new genwdg::ISubsWdg(subsystem, genType, genId, this);

    subsystemWidgets[index] = subsWdg;
    stackedLayout->addWidget(subsWdg);

    // Коннектим сигнал SubsWdg к сигналу WorkArea
    connect(subsWdg, &genwdg::ISubsWdg::signalSendUniterMessage,
            this, &WorkArea::signalSendUniterMessage);
}

void WorkArea::removeSubsystem(int index) {
    auto it = subsystemWidgets.find(index);
    if (it != subsystemWidgets.end()) {
        genwdg::ISubsWdg* subsWdg = it->second;

        disconnect(subsWdg, &genwdg::ISubsWdg::signalSendUniterMessage,
                   this, &WorkArea::signalSendUniterMessage);

        stackedLayout->removeWidget(subsWdg);
        subsWdg->deleteLater();

        subsystemWidgets.erase(it);
    }
}

void WorkArea::onSwitchTab(int index) {
    auto it = subsystemWidgets.find(index);
    if (it != subsystemWidgets.end()) {
        qDebug() << "WorkArea::onSwitchTab(): switching to index" << index;
        stackedLayout->setCurrentWidget(it->second);
    }
}


void WorkArea::onSendUniterMessage(std::shared_ptr<messages::UniterMessage> message) {
    emit signalSendUniterMessage(message);
}



} // uniter::staticwdg


