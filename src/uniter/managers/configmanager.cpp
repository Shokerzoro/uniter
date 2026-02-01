
#include "../messages/unitermessage.h"
#include "../resources/employee/employee.h"
#include "configmanager.h"
#include <QObject>
#include <memory>

namespace uniter::managers {

ConfigManager* ConfigManager::instance() {
    static ConfigManager s_instance;
    return &s_instance;
}

ConfigManager::ConfigManager() : QObject(nullptr) {
    // Инициализация, если нужна
}


void ConfigManager::onConfigProc(std::shared_ptr<resources::employees::Employee> user_)
{
    qDebug() << "ConfigManager::onConfigProc() - START";

    if (!user_) {
        qDebug() << "ConfigManager::onConfigProc() - No user, skipping configuration";
        emit signalConfigured();
        return;
    }

    user = std::move(user_);

    // Собираем список подсистем для лога
    QStringList subsystemList;
    for (const auto& assign : user->assignments) {
        subsystemList << messages::subsystemToString(assign.subsystem);
    }

    qDebug() << "ConfigManager::onConfigProc() - User:" << user->surname << user->name
             << "Subsystems:" << subsystemList.join(", ");

    for (const resources::employees::EmployeeAssignment& assign : user->assignments) {

        bool created = true;
        messages::Subsystem subsystem = assign.subsystem;
        messages::GenSubsystemType genType = messages::GenSubsystemType::NOTGEN;
        std::optional<uint64_t> genId = std::nullopt;

        if (subsystem == messages::Subsystem::GENERATIVE) {
            genType = assign.genSubsystem;
            genId = assign.genId;
        }

        emit signalSubsystemAdded(subsystem, genType, genId, created);
    }

    qDebug() << "ConfigManager::onConfigProc() - Configuration completed, emit signalConfigured()";
    emit signalConfigured();
}


} // uniter::managers
