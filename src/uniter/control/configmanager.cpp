
#include "../contract/unitermessage.h"
#include "../contract/manager/employee.h"
#include "configmanager.h"
#include "../contract_qt/qt_compat.h"
#include <QObject>
#include <QStringList>
#include <memory>

namespace uniter::control {

ConfigManager* ConfigManager::instance() {
    static ConfigManager s_instance;
    return &s_instance;
}

ConfigManager::ConfigManager() : QObject(nullptr) {
    // Инициализация, если нужна
}


void ConfigManager::onConfigProc(std::shared_ptr<contract::manager::Employee> user_)
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
        subsystemList << uniter::qt_compat::toQ(contract::subsystemToString(assign.subsystem));
    }

    qDebug() << "ConfigManager::onConfigProc() - User:"
             << uniter::qt_compat::toQ(user->surname)
             << uniter::qt_compat::toQ(user->name)
             << "Subsystems:" << subsystemList.join(", ");

    for (const contract::manager::EmployeeAssignment& assign : user->assignments) {

        bool created = true;
        contract::Subsystem subsystem = assign.subsystem;
        contract::GenSubsystem genType = contract::GenSubsystem::NOTGEN;
        std::optional<uint64_t> genId = std::nullopt;

        if (subsystem == contract::Subsystem::GENERATIVE) {
            genType = assign.genSubsystem;
            genId = assign.genId;
        }

        emit signalSubsystemAdded(subsystem, genType, genId, created);
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

    // Собираем список подсистем для лога
    QStringList subsystemList;
    for (const auto& assign : user->assignments) {
        subsystemList << uniter::qt_compat::toQ(contract::subsystemToString(assign.subsystem));
    }

    qDebug() << "ConfigManager::onClearResources() - Clearing user:"
             << uniter::qt_compat::toQ(user->surname)
             << uniter::qt_compat::toQ(user->name)
             << "Subsystems:" << subsystemList.join(", ");

    // Для каждой назначенной подсистемы отправляем событие с created = false
    for (const contract::manager::EmployeeAssignment& assign : user->assignments) {

        bool created = false; // ← в отличие от onConfigProc()
        contract::Subsystem subsystem = assign.subsystem;
        contract::GenSubsystem genType = contract::GenSubsystem::NOTGEN;
        std::optional<uint64_t> genId = std::nullopt;

        if (subsystem == contract::Subsystem::GENERATIVE) {
            genType = assign.genSubsystem;
            genId = assign.genId;
        }

        emit signalSubsystemAdded(subsystem, genType, genId, created);
    }

    // Освобождаем пользователя
    user.reset();

    qDebug() << "ConfigManager::onClearResources() - DONE, user cleared";
}



} // uniter::control
