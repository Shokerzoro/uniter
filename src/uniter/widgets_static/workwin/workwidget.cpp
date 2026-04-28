
#include "workwidget.h"
#include "../../contract_qt/qt_compat.h"
#include "../../control/uimanager.h"
#include "../../control/configmanager.h"
#include "../contract/unitermessage.h"
#include "./workbar/workbar.h"
#include "./workarea/workarea.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QDebug>
#include <map>
#include <memory>
#include <cstdint>
#include <optional>

namespace uniter::staticwdg {


WorkWdg::WorkWdg(QWidget* parent) : QWidget(parent) {

    // Creating widgets
    workbar = new WorkBar(this);
    workArea = new WorkArea(this);

    // Layout - HORIZONTAL to place the workbar on the left
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(workbar, 0);      // Left, no stretch
    layout->addWidget(workArea, 1);     // Right, stretched
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Connections WorkBar → WorkArea
    connect(workbar, &WorkBar::signalSwitchTab,
            workArea, &WorkArea::onSwitchTab);

    // Connections WorkBar → WorkWdg (messages go up to the network)
    connect(workbar, &WorkBar::signalSendUniterMessage,
            this, &WorkWdg::signalSendUniterMessage);

    // Connections WorkArea → WorkWdg (messages go up to the network)
    connect(workArea, &WorkArea::signalSendUniterMessage,
            this, &WorkWdg::signalSendUniterMessage);

    // Connection with ConfigManager
    auto ConMgr = control::ConfigManager::instance();
    connect(ConMgr, &control::ConfigManager::signalSubsystemAdded,
            this, &WorkWdg::onSubsystemAdded);
}



void WorkWdg::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message) {
    emit signalSendUniterMessage(message);
}

void WorkWdg::onSubsystemAdded(contract::Subsystem subsystem,
                               contract::GenSubsystem genType,
                               std::optional<uint64_t> genId,
                               bool created) {

    qDebug() << "WorkWdg::onSubsystemAdded() -" << subsystem << (created ? "ADD" : "REMOVE");

    if (created) {
        addSubsystem(subsystem, genType, genId);
    } else {
        removeSubsystem(subsystem, genType, genId);
    }
}

void WorkWdg::addSubsystem(contract::Subsystem subsystem,
                           contract::GenSubsystem genType,
                           std::optional<uint64_t> genId) {

    qDebug() << "WorkWdg::addSubsystem():" << subsystem;

    // Allocating an index for the new subsystem
    int index = nextIndex++;
    uint64_t genId_ = (genId == std::nullopt) ? 0 : genId.value();

    // Adding active subsystems to the map
    indexToSubsystem.insert_or_assign(
        index,
        ActiveSubsystem{subsystem, genType, genId_}
    );

    // Calling the WorkBar and WorkArea methods to add a subsystem
    workbar->addSubsystem(subsystem, genType, genId_, index);  // TODO: get the correct name
    workArea->addSubsystem(subsystem, genType, genId_, index);
}

void WorkWdg::removeSubsystem(contract::Subsystem subsystem,
                              contract::GenSubsystem genType,
                              std::optional<uint64_t> genId) {
    int index = -1;
    uint64_t genId_ = (genId == std::nullopt) ? 0 : genId.value();

    if (findIndex(subsystem, genType, genId_, index)) {
        indexToSubsystem.erase(index);
        workbar->removeSubsystem(index);
        workArea->removeSubsystem(index);
    }
}

bool WorkWdg::findIndex(contract::Subsystem subsystem,
                        contract::GenSubsystem genType,
                        std::optional<uint64_t> genId,
                        int& outIndex) const {
    uint64_t genId_ = (genId == std::nullopt) ? 0 : genId.value();

    for (const auto& [idx, active] : indexToSubsystem) {
        if (active.subsystem == subsystem &&
            active.genType == genType &&
            active.genId == genId_) {
            outIndex = idx;
            return true;
        }
    }
    return false;
}


} //uniter::staticwdg
