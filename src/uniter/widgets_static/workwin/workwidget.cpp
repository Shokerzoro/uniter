
#include "workwidget.h"
#include "../../messages/unitermessage.h"
#include "./statusbar/statusbar.h"
#include "./workarea/workarea.h"
#include <QWidget>
#include <QBoxLayout>
#include <map>
#include <memory>
#include <cstdint>
#include <optional>

namespace uniter::staticwdg {


WorkWdg::WorkWdg(QWidget* parent) : QWidget(parent) {

    // Создаём виджеты
    statusBar = new StatusBar(this);
    workArea = new WorkArea(this);

    // Компоновка
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(workArea, 1);
    layout->addWidget(statusBar, 0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Коннекты StatusBar → WorkArea
    connect(statusBar, &StatusBar::signalSwitchTab,
            workArea, &WorkArea::onSwitchTab);

    // Коннекты StatusBar → WorkWdg (сообщения идут вверх в сеть)
    connect(statusBar, &StatusBar::signalSendUniterMessage,
            this, &WorkWdg::signalSendUniterMessage);

    // Коннекты WorkArea → WorkWdg (сообщения идут вверх в сеть)
    connect(workArea, &WorkArea::signalSendUniterMessage,
            this, &WorkWdg::signalSendUniterMessage);
}




void WorkWdg::onSendUniterMessage(std::shared_ptr<messages::UniterMessage> message) {
    emit signalSendUniterMessage(message);
}

void WorkWdg::onSubsystemAdded(messages::Subsystem subsystem,
                               messages::GenSubsystemType genType,
                               std::optional<uint64_t> genId,
                               bool created) {


    if (created) {
        addSubsystem(subsystem, genType, genId);
    } else {
        removeSubsystem(subsystem, genType, genId);
    }
}

void WorkWdg::addSubsystem(messages::Subsystem subsystem,
                           messages::GenSubsystemType genType,
                           std::optional<uint64_t> genId) {

    // Выделяем индекс для новой подсистемы
    int index = nextIndex++;
    uint64_t genId_ = (genId == std::nullopt) ? 0 : genId.value();

    // Добавляем в map активных подсистем
    indexToSubsystem.insert_or_assign(
        index,
        ActiveSubsystem{subsystem, genType, genId_}
    );

    // Вызываем методы StatusBar и WorkArea для добавления подсистемы
    statusBar->addSubsystem(genType, "Subsystem", index);  // TODO: получить правильное имя
    workArea->addSubsystem(subsystem, genType, genId_, index);
}

void WorkWdg::removeSubsystem(messages::Subsystem subsystem,
                              messages::GenSubsystemType genType,
                              std::optional<uint64_t> genId) {
    int index = -1;
    uint64_t genId_ = (genId == std::nullopt) ? 0 : genId.value();

    if (findIndex(subsystem, genType, genId_, index)) {
        indexToSubsystem.erase(index);
        statusBar->removeSubsystem(index);
        workArea->removeSubsystem(index);
    }
}

bool WorkWdg::findIndex(messages::Subsystem subsystem,
                        messages::GenSubsystemType genType,
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
