
#include "employee.h"
#include <tinyxml2.h>
#include <QStringList>

namespace uniter::contract::employees {

// ---------------- Employee: каскадная сериализация ----------------
//
// Employee в БД превращается в две таблицы:
//   employees(id, name, surname, patronymic, email, ...)
//   employee_assignments(employee_id, subsystem, gen_subsystem, gen_id, permissions_csv)
// Поле permissions — uint8_t, т.к. семантика зависит от подсистемы
// (ManagerPermission, DesignPermission и т.п., см. permissions.h).
// В XML хранится как CSV-строка, чтобы не плодить вложенные теги.

void Employee::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putString(dest, "name",       name);
    putString(dest, "surname",    surname);
    putString(dest, "patronymic", patronymic);
    putString(dest, "email",      email);

    auto* doc = dest->GetDocument();
    if (!doc) return;

    auto* asgEl = doc->NewElement("Assignments");
    for (const auto& a : assignments) {
        auto* ae = doc->NewElement("Assignment");
        putInt      (ae, "subsystem",     static_cast<int>(a.subsystem));
        putInt      (ae, "gen_subsystem", static_cast<int>(a.genSubsystem));
        putOptUInt64(ae, "gen_id",        a.genId);

        // permissions как CSV (порядок сохраняется, т.к. vector)
        QStringList perms;
        perms.reserve(static_cast<int>(a.permissions.size()));
        for (auto p : a.permissions)
            perms << QString::number(static_cast<uint32_t>(p));
        putString(ae, "permissions", perms.join(","));

        asgEl->InsertEndChild(ae);
    }
    dest->InsertEndChild(asgEl);
}

void Employee::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    name       = getString(source, "name");
    surname    = getString(source, "surname");
    patronymic = getString(source, "patronymic");
    email      = getString(source, "email");

    assignments.clear();
    if (auto* asgEl = source->FirstChildElement("Assignments")) {
        for (auto* ae = asgEl->FirstChildElement("Assignment");
             ae; ae = ae->NextSiblingElement("Assignment")) {
            EmployeeAssignment a;
            a.subsystem    = static_cast<contract::Subsystem>(
                                getInt(ae, "subsystem",
                                       static_cast<int>(contract::Subsystem::PROTOCOL)));
            a.genSubsystem = static_cast<contract::GenSubsystemType>(
                                getInt(ae, "gen_subsystem",
                                       static_cast<int>(contract::GenSubsystemType::NOTGEN)));
            a.genId        = getOptUInt64(ae, "gen_id");

            const QString permsStr = getString(ae, "permissions");
            if (!permsStr.isEmpty()) {
                for (const QString& token : permsStr.split(',', Qt::SkipEmptyParts)) {
                    bool ok = false;
                    const uint32_t v = token.trimmed().toUInt(&ok);
                    if (ok) a.permissions.push_back(static_cast<uint8_t>(v));
                }
            }
            assignments.push_back(std::move(a));
        }
    }
}


} // namespace uniter::contract::employees
