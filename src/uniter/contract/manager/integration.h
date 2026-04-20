#ifndef INTEGRATION_H
#define INTEGRATION_H

#include "../resourceabstract.h"
#include "managertypes.h"
#include <QString>
#include <cstdint>
#include <optional>

namespace uniter::contract::manager {

/**
 * @brief Интеграция с внешней компанией-партнёром
 *        (ResourceType::INTEGRATION = 12, Subsystem::MANAGER).
 *
 * Генеративный ресурс: создание Integration порождает
 * GenSubsystem::INTERGATION c `GenSubsystemId == integration.id`.
 * Под каждой активной интеграцией появляется свой набор IntegrationTask
 * (см. contract/integration/integrationtask.h) — по одной задаче на каждую
 * передачу конкретного ресурса во внешнюю систему.
 *
 * Контактная информация партнёра хранится как свободные строки. TODO:
 * выделить отдельный ресурс PartnerCompany, если понадобится справочник
 * компаний и повторное использование реквизитов между интеграциями.
 */
class Integration : public ResourceAbstract
{
public:
    Integration() = default;

    Integration(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString name_,
        QString description_,
        QString partner_company_,
        QString partner_contact_,
        IntegrationStatus status_,
        std::optional<uint64_t> initiator_employee_id_ = std::nullopt)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          name(std::move(name_)),
          description(std::move(description_)),
          partner_company(std::move(partner_company_)),
          partner_contact(std::move(partner_contact_)),
          status(status_),
          initiator_employee_id(initiator_employee_id_)
    {}

    // Идентификация
    QString name;           // Человекочитаемое название интеграции
    QString description;    // Назначение / описание процесса

    // Партнёр
    QString partner_company;   // Название компании-партнёра
    QString partner_contact;   // Контакт (email / телефон / ник) — свободная строка

    // Состояние
    IntegrationStatus status = IntegrationStatus::DRAFT;

    // Кто инициатор (FK → employees.id). Опционален — могут создавать
    // системные интеграции без конкретного сотрудника.
    std::optional<uint64_t> initiator_employee_id;

    friend bool operator==(const Integration& a, const Integration& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.name                   == b.name
            && a.description            == b.description
            && a.partner_company        == b.partner_company
            && a.partner_contact        == b.partner_contact
            && a.status                 == b.status
            && a.initiator_employee_id  == b.initiator_employee_id;
    }
    friend bool operator!=(const Integration& a, const Integration& b) { return !(a == b); }
};

} // namespace uniter::contract::manager

#endif // INTEGRATION_H
