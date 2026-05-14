
#include "../contract/unitermessage.h"
#include "../contract/manager/employee.h"
#include "configmanager.h"
#include <QObject>
#include <memory>

namespace control {

ConfigManager* ConfigManager::instance() {
    static ConfigManager s_instance;
    return &s_instance;
}

ConfigManager::ConfigManager() : QObject(nullptr) {
    // Инициализация, если нужна
}


void ConfigManager::onConfigProc(std::shared_ptr<contract::employees::Employee> user_)
{
    qDebug() << "ConfigManager::onConfigProc() - START";

    if (!user_) {
        qDebug() << "ConfigManager::onConfigProc() - No user, skipping configuration";
        emit signalConfigured();
        return;
    }

    user = std::move(user_);


    for (const contract::employees::EmployeeAssignment& assign : user->assignments) {

        bool created = true;
        contract::Subsystem subsystem = assign.subsystem;
        std::optional<uint64_t> subsystemInstanceId = assign.subsystemInstanceId;

        emit signalSubsystemAdded(subsystem, subsystemInstanceId, created);
    }

    qDebug() << "ConfigManager::onConfigProc() - Configuration completed, emit signalConfigured()";
    emit signalConfigured();
}

void ConfigManager::onClearResources()
{
    qDebug() << "ConfigManager::onClearResources() - START";

    if (!user) {
        qDebug() << "ConfigManager::onClearResources() - No user, nothing to clear";
        return;
    }


    // Для каждой назначенной подсистемы отправляем событие с created = false
    for (const contract::employees::EmployeeAssignment& assign : user->assignments) {

        bool created = false; // ← в отличие от onConfigProc()
        contract::Subsystem subsystem = assign.subsystem;
        std::optional<uint64_t> subsystemInstanceId = assign.subsystemInstanceId;

        emit signalSubsystemAdded(subsystem, subsystemInstanceId, created);
    }

    // Освобождаем пользователя
    user.reset();

    qDebug() << "ConfigManager::onClearResources() - DONE, user cleared";
}



} // control
