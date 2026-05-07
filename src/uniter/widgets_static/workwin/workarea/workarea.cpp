
#include "../../../contract/unitermessage.h"
#include "../../../widgets_generative/subsystemtab.h"
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

void WorkArea::addSubsystem(contract::Subsystem subsystem,
                            contract::GenSubsystemType genType,
                            uint64_t genId,
                            int index) {

    qDebug() << "WorkArea::addSubsystem():" << subsystem;

    auto* subsWdg = new genwdg::SubsystemTab(subsystem, genType, genId, this);

    subsystemWidgets[index] = subsWdg;
    stackedLayout->addWidget(subsWdg);

    // Коннектим сигнал SubsWdg к сигналу WorkArea
    connect(subsWdg, &genwdg::SubsystemTab::signalSendUniterMessage,
            this, &WorkArea::signalSendUniterMessage);
}

void WorkArea::removeSubsystem(int index) {
    auto it = subsystemWidgets.find(index);
    if (it != subsystemWidgets.end()) {
        genwdg::SubsystemTab* subsWdg = it->second;

        disconnect(subsWdg, &genwdg::SubsystemTab::signalSendUniterMessage,
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


void WorkArea::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message) {
    emit signalSendUniterMessage(message);
}



} // uniter::staticwdg


