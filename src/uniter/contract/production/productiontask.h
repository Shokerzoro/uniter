#ifndef PRODUCTIONTASK_H
#define PRODUCTIONTASK_H

#include "../uniterprotocol.h"
#include "../resourceabstract.h"
#include "productiontypes.h"
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>

namespace uniter::contract::production {

/**
 * @brief Производственное задание
 *        (ResourceType::PRODUCTION_TASK = 70,
 *         Subsystem::GENERATIVE + GenSubsystem::PRODUCTION).
 *
 * Маппится на таблицу `production_task` (см. docs/db/production.md).
 *
 * Ключевые FK (согласно PRODUCTION.pdf):
 *   - `plant_id`             — площадка-владелец (генеративная подсистема)
 *   - `snapshot_original_id` — снэпшот, под который задание было создано
 *                              (фиксирует исходный состав КД; неизменяемый)
 *   - `snapshot_current_id`  — актуальный снэпшот, по которому сейчас
 *                              ведётся производство (обновляется при утверждении
 *                              новой APPROVED-версии проекта)
 *
 * В момент создания `snapshot_original_id == snapshot_current_id`.
 * Они расходятся, когда проект получает более новый APPROVED Snapshot и
 * задание переключается на него; исходный снэпшот сохраняется для
 * воспроизводимости.
 *
 * Оба snapshot_*_id указывают на `pdm_snapshot.id` — НЕ на `design_*`
 * напрямую, потому что снэпшот неизменяем, а `design_*` меняются свободно.
 * См. docs/db/pdm_design_logic.md.
 *
 * Древовидные поля (ProductionAssemblyNode / ProductionPartNode) удалены:
 * состав задания читается через snapshot_current_id → pdm::Assembly и его
 * конфигурации. Если понадобится трекать прогресс по узлам — это отдельные
 * ресурсы ProductionTaskAssemblyProgress / ProductionTaskPartProgress
 * (вне текущей модели).
 */
class ProductionTask : public ResourceAbstract
{
public:
    ProductionTask()
        : ResourceAbstract(Subsystem::GENERATIVE, GenSubsystemType::PRODUCTION,
                           ResourceType::PRODUCTION_TASK) {}
    ProductionTask(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t plant_id_,
        uint64_t snapshot_original_id_,
        uint64_t snapshot_current_id_,
        QString  code_        = {},
        QString  name_        = {},
        uint64_t quantity_    = 0,
        TaskStatus status_    = TaskStatus::PLANNED)
        : ResourceAbstract(Subsystem::GENERATIVE, GenSubsystemType::PRODUCTION,
                           ResourceType::PRODUCTION_TASK,
                           s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          plant_id(plant_id_),
          snapshot_original_id(snapshot_original_id_),
          snapshot_current_id(snapshot_current_id_),
          code(std::move(code_)),
          name(std::move(name_)),
          quantity(quantity_),
          status(status_)
    {}

    // --- Ключевые FK (структурная схема) ------------------------------------
    uint64_t plant_id             = 0;   // FK → manager_plant.id
    uint64_t snapshot_original_id = 0;   // FK → pdm_snapshot.id (исходная КД)
    uint64_t snapshot_current_id  = 0;   // FK → pdm_snapshot.id (актуальная КД)

    // --- Прикладные атрибуты (не в структурной схеме, но нужны ERP/UI) ------
    // TODO(на подумать): вынести в отдельный ресурс ProductionTaskAttributes,
    // если структурная схема принципиально не должна их содержать.
    QString    code;
    QString    name;
    uint64_t   quantity = 0;
    TaskStatus status   = TaskStatus::PLANNED;

    // Тайминги (прикладные)
    QDateTime                planned_start;
    QDateTime                planned_end;
    std::optional<QDateTime> actual_start;
    std::optional<QDateTime> actual_end;

    friend bool operator==(const ProductionTask& a, const ProductionTask& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.plant_id             == b.plant_id
            && a.snapshot_original_id == b.snapshot_original_id
            && a.snapshot_current_id  == b.snapshot_current_id
            && a.code                 == b.code
            && a.name                 == b.name
            && a.quantity             == b.quantity
            && a.status               == b.status
            && a.planned_start        == b.planned_start
            && a.planned_end          == b.planned_end
            && a.actual_start         == b.actual_start
            && a.actual_end           == b.actual_end;
    }
    friend bool operator!=(const ProductionTask& a, const ProductionTask& b) { return !(a == b); }
};

} // namespace uniter::contract::production

#endif // PRODUCTIONTASK_H
