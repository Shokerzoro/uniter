#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include "../uniterprotocol.h"
#include "../resourceabstract.h"
#include "permissions.h"
#include <QString>
#include <QDateTime>
#include <optional>
#include <cstdint>
#include <vector>

namespace uniter::contract::manager {

/**
 * @brief Назначение сотрудника на подсистему.
 *
 * Сворачивается из трёх таблиц БД (см. docs/db/manager.md):
 *
 *   manager/employee_assignment — сама запись назначения
 *     (id PK, subsystem INT, gensubsystem INT, gensubsystem_id BIGINT NULL)
 *   manager/permissions         — список разрешений для этого назначения
 *     (id PK, assignment_id FK → employee_assignment.id, permission INT)
 *   manager/employee_assignment_link — M:N junction
 *     (employee_id FK, assignment_id FK)
 *
 * В рантайме всё свёрнуто в `EmployeeAssignment`:
 *   subsystem / genSubsystem / genId — поля самого `employee_assignment`;
 *   permissions — свёрнутая выборка `manager/permissions` по
 *                 `assignment_id = this.id` (у рантайм-объекта хранится
 *                 как vector<uint8_t>; конкретные значения зависят от
 *                 подсистемы — ManagerPermission / DesignPermission / …).
 *
 * FK `assignment_id` в `manager/permissions` в рантайм-класс не
 * переносится: Permissions живут внутри своего EmployeeAssignment и
 * знают «своего родителя» через положение в контейнере. Аналогично
 * junction `employee_assignment_link` не имеет отдельного класса —
 * при чтении Employee его assignments подтягиваются join-ом.
 *
 * Ресурсы в uniterprotocol.h:
 *   ResourceType::EMPLOYEES             = 10  — таблица employee
 *   ResourceType::EMPLOYEE_ASSIGNMENT   = ... — таблица employee_assignment
 *   ResourceType::PERMISSION            = ... — таблица permissions
 *   (плюс связочная таблица employee_assignment_link — добавляется по
 *   необходимости, если серверу нужно CRUD на сам факт назначения
 *   сотрудника на assignment отдельно от самого assignment)
 */
struct EmployeeAssignment {
    contract::Subsystem subsystem;
    contract::GenSubsystemType genSubsystem = contract::GenSubsystemType::NOTGEN;
    std::optional<uint64_t> genId = std::nullopt;
    std::vector<uint8_t> permissions;

    friend bool operator==(const EmployeeAssignment& a, const EmployeeAssignment& b) {
        return a.subsystem    == b.subsystem
            && a.genSubsystem == b.genSubsystem
            && a.genId        == b.genId
            && a.permissions  == b.permissions;
    }
    friend bool operator!=(const EmployeeAssignment& a, const EmployeeAssignment& b) { return !(a == b); }
};

/**
 * @brief Сотрудник / пользователь (ResourceType::EMPLOYEES = 10).
 *
 * Subsystem::MANAGER. Логин/пароль остаются на стороне сервера (этот класс
 * отражает учётную запись, а не credentials).
 *
 * Сворачивание:
 *   manager/employee                 → сам Employee
 *   manager/employee_assignment_link → выборка id всех assignment-ов сотрудника
 *   manager/employee_assignment      → поля каждого EmployeeAssignment
 *   manager/permissions              → EmployeeAssignment::permissions
 *
 * Всё это укладывается в `Employee::assignments`. FK (junction.employee_id,
 * junction.assignment_id, permissions.assignment_id) в рантайм-классы
 * не переносятся.
 */
class Employee : public ResourceAbstract {
public:
    Employee() = default;

    Employee(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString name_,
        QString surname_,
        QString patronymic_,
        QString email_,
        std::vector<EmployeeAssignment> assignments_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
        name(std::move(name_)),
        surname(std::move(surname_)),
        patronymic(std::move(patronymic_)),
        email(std::move(email_)),
        assignments(std::move(assignments_)) {}

    QString name;
    QString surname;
    QString patronymic;
    QString email;
    // Может быть назначен на несколько подсистем — свёрнутая выборка
    // manager/employee_assignment (+permissions) через junction
    // manager/employee_assignment_link.
    std::vector<EmployeeAssignment> assignments;

    friend bool operator==(const Employee& a, const Employee& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.name        == b.name
            && a.surname     == b.surname
            && a.patronymic  == b.patronymic
            && a.email       == b.email
            && a.assignments == b.assignments;
    }
    friend bool operator!=(const Employee& a, const Employee& b) { return !(a == b); }
};

} // namespace uniter::contract::manager

#endif // EMPLOYEE_H
