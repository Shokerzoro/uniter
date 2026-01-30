#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include "../../messages/unitermessage.h"
#include "../resourceabstract.h"
#include "permissions.h"
#include <tinyxml2.h>
#include <QString>
#include <QDateTime>
#include <optional>
#include <cstdint>

namespace uniter::resources::employees {


struct EmployeeAssignment {
    messages::Subsystem subsystem;
    messages::GenSubsystemType genSubsystem = messages::GenSubsystemType::NOTGEN;
    std::optional<uint64_t> genId = std::nullopt;
    std::vector<uint8_t> permissions;
};

class Employee : public ResourceAbstract {
public:
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

    // Сериалиализация десериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // employees

#endif // EMPLOYEE_H
