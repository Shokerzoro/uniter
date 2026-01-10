
#include "../resources/employee/employee.h"
#include <memory>

namespace uniter::managers {

// Управляет созданием подсистем
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    ConfigManager() {}
    virtual ~ConfigManager() {}

    // Генерация конфигурации
public slots:
    void GenerateConfiguration(std::shared_ptr<resources::employees::Employee> User);
};


} // managers
