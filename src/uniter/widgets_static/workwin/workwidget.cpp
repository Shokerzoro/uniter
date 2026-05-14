
#include "workwidget.h"
#include "../../control/uimanager.h"
#include "../../control/configmanager.h"
#include "../../contract/unitermessage.h"
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

    // Создаём виджеты
    workbar = new WorkBar(this);
    workArea = new WorkArea(this);

    // Компоновка - ГОРИЗОНТАЛЬНАЯ для размещения workbar слева
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(workbar, 0);      // Слева, без растяжения
    layout->addWidget(workArea, 1);     // Справа, растягивается
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Коннекты WorkBar → WorkArea
    connect(workbar, &WorkBar::signalSwitchTab,
            workArea, &WorkArea::onSwitchTab);

    // Коннекты WorkBar → WorkWdg (сообщения идут вверх в сеть)
    connect(workbar, &WorkBar::signalSendUniterMessage,
            this, &WorkWdg::signalSendUniterMessage);

    // Коннекты WorkArea → WorkWdg (сообщения идут вверх в сеть)
    connect(workArea, &WorkArea::signalSendUniterMessage,
            this, &WorkWdg::signalSendUniterMessage);

    // Коннект с ConfigManager
    auto ConMgr = control::ConfigManager::instance();
    connect(ConMgr, &control::ConfigManager::signalSubsystemAdded,
            this, &WorkWdg::onSubsystemAdded);
}



void WorkWdg::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message) {
    emit signalSendUniterMessage(message);
}

void WorkWdg::onSubsystemAdded(contract::Subsystem subsystem,
                               std::optional<uint64_t> subsystemInstanceId,
                               bool created) {

    qDebug() << "WorkWdg::onSubsystemAdded() -" << subsystem << (created ? "ADD" : "REMOVE");

    if (created) {
        addSubsystem(subsystem, subsystemInstanceId);
    } else {
        removeSubsystem(subsystem, subsystemInstanceId);
    }
}

void WorkWdg::addSubsystem(contract::Subsystem subsystem,
                           std::optional<uint64_t> subsystemInstanceId) {

    qDebug() << "WorkWdg::addSubsystem():" << subsystem;

    // Выделяем индекс для новой подсистемы
    int index = nextIndex++;
    // Добавляем в map активных подсистем
    indexToSubsystem.insert_or_assign(
        index,
        ActiveSubsystem{subsystem, subsystemInstanceId}
    );

    // Вызываем методы WorkBar и WorkArea для добавления подсистемы
    workbar->addSubsystem(subsystem, subsystemInstanceId, index);  // TODO: получить правильное имя
    workArea->addSubsystem(subsystem, subsystemInstanceId, index);
}

void WorkWdg::removeSubsystem(contract::Subsystem subsystem,
                              std::optional<uint64_t> subsystemInstanceId) {
    int index = -1;

    if (findIndex(subsystem, subsystemInstanceId, index)) {
        indexToSubsystem.erase(index);
        workbar->removeSubsystem(index);
        workArea->removeSubsystem(index);
    }
}

bool WorkWdg::findIndex(contract::Subsystem subsystem,
                        std::optional<uint64_t> subsystemInstanceId,
                        int& outIndex) const {
    for (const auto& [idx, active] : indexToSubsystem) {
        if (active.subsystem == subsystem &&
            active.subsystemInstanceId == subsystemInstanceId) {
            outIndex = idx;
            return true;
        }
    }
    return false;
}


} //uniter::staticwdg
