
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
                               contract::GenSubsystemType genType,
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
                           contract::GenSubsystemType genType,
                           std::optional<uint64_t> genId) {

    qDebug() << "WorkWdg::addSubsystem():" << subsystem;

    // Выделяем индекс для новой подсистемы
    int index = nextIndex++;
    uint64_t genId_ = (genId == std::nullopt) ? 0 : genId.value();

    // Добавляем в map активных подсистем
    indexToSubsystem.insert_or_assign(
        index,
        ActiveSubsystem{subsystem, genType, genId_}
    );

    // Вызываем методы WorkBar и WorkArea для добавления подсистемы
    workbar->addSubsystem(subsystem, genType, genId_, index);  // TODO: получить правильное имя
    workArea->addSubsystem(subsystem, genType, genId_, index);
}

void WorkWdg::removeSubsystem(contract::Subsystem subsystem,
                              contract::GenSubsystemType genType,
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
                        contract::GenSubsystemType genType,
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
