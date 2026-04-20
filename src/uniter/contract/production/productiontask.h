#ifndef PRODUCTIONTASK_H
#define PRODUCTIONTASK_H

#include "../resourceabstract.h"
#include "productiontypes.h"
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>
#include <vector>

namespace uniter::contract::production {

/**
 * @brief Производственное задание
 *        (ResourceType::PRODUCTION_TASK = 70,
 *         Subsystem::GENERATIVE + GenSubsystem::PRODUCTION).
 *
 * Перенесён из contract/plant/task.h и переименован Task → ProductionTask
 * для соответствия структуре подсистемы PRODUCTION.
 *
 * Ссылается на проект через `project_id` и на зафиксированный Snapshot
 * через `snapshot_id` (pdm_design_architecture.md §3): если после создания
 * задания проект получит новый APPROVED Snapshot, задание продолжит
 * ссылаться на снэпшот, действовавший на момент создания.
 *
 * `plant_id` — FK → plants.id (площадка-владелец генеративной подсистемы).
 */
class ProductionTask : public ResourceAbstract
{
public:
    ProductionTask() = default;
    ProductionTask(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString code_,
        QString name_,
        uint64_t project_id_,
        uint64_t snapshot_id_,
        uint64_t quantity_,
        TaskStatus status_,
        uint64_t plant_id_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          code(std::move(code_)),
          name(std::move(name_)),
          project_id(project_id_),
          snapshot_id(snapshot_id_),
          plant_id(plant_id_),
          quantity(quantity_),
          status(status_)
    {}

    // Идентификация задания
    QString  code;
    QString  name;

    // Связь с конструкторской подсистемой и площадкой
    uint64_t project_id  = 0;   // FK → projects.id
    uint64_t snapshot_id = 0;   // FK → snapshots.id (зафиксированный состав)
    uint64_t plant_id    = 0;   // FK → plants.id   (площадка выполнения)

    // Количественные параметры
    uint64_t   quantity = 0;
    TaskStatus status   = TaskStatus::PLANNED;

    // Дерево производственных узлов (сборки + детали).
    // TODO(refactor/split): см. productiontypes.h — перевести в отдельные
    // ресурсы-узлы по FK production_task_id.
    std::vector<ProductionAssemblyNode> root_assemblies;

    // Период выполнения
    QDateTime                planned_start;
    QDateTime                planned_end;
    std::optional<QDateTime> actual_start;
    std::optional<QDateTime> actual_end;

    friend bool operator==(const ProductionTask& a, const ProductionTask& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.code            == b.code
            && a.name            == b.name
            && a.project_id      == b.project_id
            && a.snapshot_id     == b.snapshot_id
            && a.plant_id        == b.plant_id
            && a.quantity        == b.quantity
            && a.status          == b.status
            && a.root_assemblies == b.root_assemblies
            && a.planned_start   == b.planned_start
            && a.planned_end     == b.planned_end
            && a.actual_start    == b.actual_start
            && a.actual_end      == b.actual_end;
    }
    friend bool operator!=(const ProductionTask& a, const ProductionTask& b) { return !(a == b); }
};

} // namespace uniter::contract::production

#endif // PRODUCTIONTASK_H
