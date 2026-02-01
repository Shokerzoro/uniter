#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "../contract/employee/employee.h"
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
    // Приватный конструктор
    ConfigManager();

    // Запрет копирования и перемещения
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    // Текущая конфигурация
    std::shared_ptr<contract::employees::Employee> user;

public:
    // Единственная точка доступа к синглтону
    static ConfigManager* instance();

    ~ConfigManager() override = default;

public slots:
    // Генерация конфигурации по данным пользователя
    void onConfigProc(std::shared_ptr<contract::employees::Employee> User);

signals:
    // Уведомление AppManager о завершении конфигурирования
    void signalConfigured();

    // Для каждой назначенной пользователю подсистемы
    void signalSubsystemAdded(contract::Subsystem subsystem,
                              contract::GenSubsystemType genType,
                              std::optional<uint64_t> genId,
                              bool created);
};

} // namespace uniter::control

#endif // CONFIGMANAGER_H
