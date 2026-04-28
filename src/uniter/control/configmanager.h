#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "../contract/manager/employee.h"
#include "../contract/unitermessage.h"
#include <QObject>
#include <memory>
#include <optional>
#include <cstdint>

namespace uniter::control {

class ConfigManager : public QObject
{
    Q_OBJECT

private:
    // Private constructor
    ConfigManager();

    // Prohibition of copying and moving
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    // Current configuration
    std::shared_ptr<contract::manager::Employee> user;

public:
    // Singleton singleton access point
    static ConfigManager* instance();

    ~ConfigManager() override = default;

public slots:
    // Generating configuration according to user data
    void onConfigProc(std::shared_ptr<contract::manager::Employee> User);
    void onClearResources();
signals:
    // Notify AppManager that configuration is complete
    void signalConfigured();

    // For each user-assigned subsystem
    void signalSubsystemAdded(contract::Subsystem subsystem,
                              contract::GenSubsystem genType,
                              std::optional<uint64_t> genId,
                              bool created);
};

} // namespace uniter::control

#endif // CONFIGMANAGER_H
