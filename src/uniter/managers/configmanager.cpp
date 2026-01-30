
#include "../messages/unitermessage.h"
#include "../resources/employee/employee.h"
#include "configmanager.h"
#include <QObject>
#include <memory>

namespace uniter::managers {

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent) {

}

void ConfigManager::onConfigProc(std::shared_ptr<resources::employees::Employee> user_)
{
    if (!user_) {
        emit signalConfigured();
        return;
    }

    user = std::move(user_);

    for (const resources::employees::EmployeeAssignment& assign : user->assignments) {

        bool created = true;
        messages::Subsystem subsystem = assign.subsystem;
        messages::GenSubsystemType genType = messages::GenSubsystemType::NOTGEN;
        std::optional<uint64_t> genId = std::nullopt;

        if (subsystem == messages::Subsystem::GENERATIVE) {
            messages::GenSubsystemType genType = assign.genSubsystem;
            std::optional<uint64_t> genId = assign.genId;
        }

        emit signalSubsystemAdded(subsystem, genType, genId, created);
    }

    emit signalConfigured();
}

}
