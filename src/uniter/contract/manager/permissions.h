#ifndef MANAGER_PERMISSIONS_H
#define MANAGER_PERMISSIONS_H

#include <cstdint>

namespace uniter::contract::manager {

// NOTE(refactor): локальный enum Subsystem в permissions.h дублирует
// uniter::contract::Subsystem из uniterprotocol.h. Сохранён для
// совместимости с существующим Employee.assignments, где permissions
// хранятся как vector<uint8_t> без привязки к внешнему enum.
// TODO: унифицировать с contract::Subsystem и удалить этот дубликат.
enum class Subsystem : uint8_t {
    MANAGER     = 0,
    MATERIALS   = 1,
    DESIGN      = 2,
    PURCHASES   = 3,
    PRODUCTION  = 5,
    INTEGRATION = 6
};

enum class ManagerPermission : uint8_t {
    // Операции с сотрудниками
    CREATE_EMPLOYEE     = 0,
    UPDATE_EMPLOYEE     = 1,
    DELETE_EMPLOYEE     = 2,
    ASSIGN_PERMISSIONS  = 3,

    // Управление производственными подразделениями
    CREATE_PRODUCTION_UNIT = 4,
    UPDATE_PRODUCTION_UNIT = 5,
    DELETE_PRODUCTION_UNIT = 6,

    // Управление интеграциями
    CREATE_INTEGRATION  = 7,
    UPDATE_INTEGRATION  = 8,
    DELETE_INTEGRATION  = 9,
};

enum class MaterialsPermission : uint8_t {
    CREATE_TEMPLATE = 0,
    UPDATE_TEMPLATE = 1,
    DELETE_TEMPLATE = 2
};

enum class DesignPermission : uint8_t {
    CREATE_PROJECT       = 0,
    EDIT_OWN_PROJECT     = 1,
    APPROVE_OWN_PROJECT  = 2,
    EDIT_ANY_PROJECT     = 3,
    APPROVE_ANY_PROJECT  = 4
};

enum class PurchasesPermission : uint8_t {
    CREATE_PURCHASE  = 0,
    UPDATE_PURCHASE  = 1,
    APPROVE_PURCHASE = 2,
    CONFIRM_DELIVERY = 3
};

enum class ProductionPermission : uint8_t {
    CREATE_TASK        = 0,  // Создание производственных заданий
    UPDATE_TASK        = 1,  // Изменение параметров задания
    CREATE_INVENTORY   = 2,  // Оприходование материалов (поступление на склад)
    WRITE_OFF_INVENTORY = 3  // Списание материалов только на потери/порчу
                             // (списание на производство - автоматически через UPDATE_TASK)
};

enum class IntegrationPermission : uint8_t {
    CREATE_INTEGRATION_TASK = 0, // Создание задачи на передачу данных партнёру
    UPDATE_INTEGRATION_TASK = 1, // Правка/повторная отправка
    CANCEL_INTEGRATION_TASK = 2, // Отмена
};

} // namespace uniter::contract::manager

// Backward-compat: старые потребители (appmanager, configmanager, serverconnector,
// tests/appmanagertest.cpp) используют namespace uniter::contract::employees.
// Alias сохраняет их работоспособность без точечных правок include/usages.
namespace uniter::contract {
namespace employees = manager;
}

#endif // MANAGER_PERMISSIONS_H
