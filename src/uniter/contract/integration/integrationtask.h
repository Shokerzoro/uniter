#ifndef INTEGRATIONTASK_H
#define INTEGRATIONTASK_H

#include "../uniterprotocol.h"
#include "../resourceabstract.h"
#include "integrationtypes.h"
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>

namespace uniter::contract::integration {

/**
 * @brief Задача на передачу конкретного ресурса внешнему партнёру
 *        (ResourceType::INTEGRATION_TASK = 80,
 *         Subsystem::GENERATIVE + GenSubsystem::INTERGATION).
 *
 * Задача = «выгрузить ресурс X из подсистемы S в рамках интеграции I
 * партнёру». Ссылается на породившую Integration через `integration_id`
 * (FK → integrations.id == generative subsystem instance).
 *
 * Целевой ресурс идентифицируется тройкой:
 *   (target_subsystem, target_resource_type, foreign_resource_id).
 * Используем uniter::contract::Subsystem и ResourceType из uniterprotocol.h,
 * чтобы не плодить свои enum-ы — это стандартная модель «адреса» ресурса
 * в Uniter.
 *
 * TODO(refactor): если выяснится, что за одну IntegrationTask реально
 * нужно передавать несколько ресурсов разных типов, разбить на отдельный
 * ресурс IntegrationTaskItem с FK integration_task_id.
 */
class IntegrationTask : public ResourceAbstract
{
public:
    IntegrationTask() = default;

    IntegrationTask(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t integration_id_,
        contract::Subsystem target_subsystem_,
        contract::ResourceType target_resource_type_,
        uint64_t foreign_resource_id_,
        IntegrationTaskStatus status_,
        QString payload_ref_ = {},
        std::optional<QDateTime> planned_at_ = std::nullopt,
        std::optional<QDateTime> transmitted_at_ = std::nullopt,
        QString last_error_ = {})
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          integration_id(integration_id_),
          target_subsystem(target_subsystem_),
          target_resource_type(target_resource_type_),
          foreign_resource_id(foreign_resource_id_),
          status(status_),
          payload_ref(std::move(payload_ref_)),
          planned_at(planned_at_),
          transmitted_at(transmitted_at_),
          last_error(std::move(last_error_))
    {}

    // Связь с генеративной подсистемой
    uint64_t integration_id = 0; // FK → integrations.id

    // «Адрес» передаваемого ресурса
    contract::Subsystem    target_subsystem     = contract::Subsystem::PROTOCOL;
    contract::ResourceType target_resource_type = contract::ResourceType::DEFAULT;
    uint64_t               foreign_resource_id  = 0; // id ресурса в локальной БД

    // Состояние передачи
    IntegrationTaskStatus status = IntegrationTaskStatus::PENDING;

    // Опциональная ссылка на сериализованный payload (например, MinIO object_key
    // или хэш экспортированного файла). Свободная строка: содержимое определяет
    // конкретный коннектор партнёра. TODO: перевести на отдельный ресурс-файл
    // (IntegrationPayload) с DocLink-подобной семантикой.
    QString payload_ref;

    // Тайминги
    std::optional<QDateTime> planned_at;
    std::optional<QDateTime> transmitted_at;

    // Диагностика. При status=FAILED сюда складывается последнее сообщение
    // об ошибке. Пустая при успешных статусах.
    QString last_error;

    friend bool operator==(const IntegrationTask& a, const IntegrationTask& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.integration_id        == b.integration_id
            && a.target_subsystem      == b.target_subsystem
            && a.target_resource_type  == b.target_resource_type
            && a.foreign_resource_id   == b.foreign_resource_id
            && a.status                == b.status
            && a.payload_ref           == b.payload_ref
            && a.planned_at            == b.planned_at
            && a.transmitted_at        == b.transmitted_at
            && a.last_error            == b.last_error;
    }
    friend bool operator!=(const IntegrationTask& a, const IntegrationTask& b) { return !(a == b); }
};

} // namespace uniter::contract::integration

#endif // INTEGRATIONTASK_H
