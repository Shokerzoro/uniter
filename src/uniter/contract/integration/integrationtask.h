#ifndef INTEGRATIONTASK_H
#define INTEGRATIONTASK_H

#include "../uniterprotocol.h"
#include "../resourceabstract.h"
#include <cstdint>

namespace uniter::contract::integration {

/**
 * @brief Задача на выгрузку конкретного ресурса внешнему партнёру
 *        (ResourceType::INTEGRATION_TASK = 80,
 *         Subsystem::GENERATIVE + GenSubsystem::INTERGATION).
 *
 * Маппится на таблицу `integration_task` (см. docs/db/integration.md).
 *
 * Структура ресурса сведена к минимуму по схеме БД (INTEGRATION.pdf):
 *   id | integration_id | target_subsystem | target_resource_type | any_resource_id
 *
 * Целевой ресурс идентифицируется тройкой:
 *   (target_subsystem, target_resource_type, any_resource_id).
 * Это полиморфный FK: таблица-назначение определяется парой
 * (subsystem, resource_type) в рантайме; ссылочную целостность проверяет
 * DataManager при CREATE/UPDATE.
 *
 * TODO(на подумать): если понадобится отслеживание попыток/статуса
 * передачи — вынести в отдельный ресурс IntegrationTaskAttempt с FK
 * integration_task_id (история). Сейчас сама задача — это только
 * «что и куда передать», без состояния передачи.
 */
class IntegrationTask : public ResourceAbstract
{
public:
    IntegrationTask()
        : ResourceAbstract(Subsystem::GENERATIVE, GenSubsystemType::INTERGATION,
                           ResourceType::INTEGRATION_TASK) {}

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
        uint64_t any_resource_id_)
        : ResourceAbstract(Subsystem::GENERATIVE, GenSubsystemType::INTERGATION,
                           ResourceType::INTEGRATION_TASK,
                           s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          integration_id(integration_id_),
          target_subsystem(target_subsystem_),
          target_resource_type(target_resource_type_),
          any_resource_id(any_resource_id_)
    {}

    // Порождающая Integration (Subsystem::MANAGER, ResourceType::INTEGRATION = 12)
    uint64_t integration_id = 0; // FK → manager_integration.id

    // Полиморфный «адрес» передаваемого ресурса.
    // (target_subsystem, target_resource_type) → таблица;
    // any_resource_id → строка в этой таблице.
    contract::Subsystem    target_subsystem     = contract::Subsystem::PROTOCOL;
    contract::ResourceType target_resource_type = contract::ResourceType::DEFAULT;
    uint64_t               any_resource_id      = 0;

    friend bool operator==(const IntegrationTask& a, const IntegrationTask& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.integration_id        == b.integration_id
            && a.target_subsystem      == b.target_subsystem
            && a.target_resource_type  == b.target_resource_type
            && a.any_resource_id       == b.any_resource_id;
    }
    friend bool operator!=(const IntegrationTask& a, const IntegrationTask& b) { return !(a == b); }
};

} // namespace uniter::contract::integration

#endif // INTEGRATIONTASK_H
