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
 * TODO(refactor): вектор EmployeeAssignment внутри Employee — это embedded
 * one-to-many. Для реляционной БД правильнее выделить в отдельный ресурс
 * EmployeeAssignment (таблица `employee_assignments`), как советует
 * user (раздробить, а не сомкать). Сделаем после первой итерации,
 * когда будут реализованы базовые CRUD.
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
    // Может быть назначен на несколько подсистем
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
