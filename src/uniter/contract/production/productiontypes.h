#ifndef PRODUCTIONTYPES_H
#define PRODUCTIONTYPES_H

#include <cstdint>
#include <vector>
#include <optional>
#include <QDateTime>

namespace uniter::contract::production {

/**
 * @brief Статус производственного задания.
 *
 * Общий enum для ProductionTask и его дочерних узлов
 * (ProductionAssemblyNode / ProductionPartNode) — для аналитики ERP
 * удобно оперировать единым типом статуса.
 */
enum class TaskStatus : uint8_t {
    PLANNED     = 0, // Запланировано
    IN_PROGRESS = 1, // В работе
    PAUSED      = 2, // Приостановлено
    COMPLETED   = 3, // Завершено
    CANCELLED   = 4  // Отменено
};

/**
 * @brief Узел «деталь» в дереве производственного задания.
 *
 * Сейчас — embedded struct. TODO(refactor/split): выделить в отдельный
 * ресурс ProductionTaskPartNode с FK production_task_id, чтобы не
 * денормализовывать дерево внутри одной записи (пользователь просил
 * предпочитать раздробление). Требует одновременной миграции
 * ProductionAssemblyNode.
 */
struct ProductionPartNode
{
    // Идентификация (ссылки на ресурсы конструктора)
    uint64_t assembly_id = 0;   // FK → assemblies.id
    uint64_t project_id  = 0;   // FK → projects.id
    uint64_t part_id     = 0;   // FK → parts.id

    // Количественные данные
    TaskStatus status              = TaskStatus::PLANNED;
    uint64_t   quantity_planned    = 0;
    uint64_t   quantity_completed  = 0;

    // Планируемые / фактические даты для аналитики
    QDateTime                planned_start;
    QDateTime                planned_end;
    std::optional<QDateTime> actual_start;
    std::optional<QDateTime> actual_end;

    QDateTime updated_at;

    friend bool operator==(const ProductionPartNode& a, const ProductionPartNode& b) {
        return a.assembly_id        == b.assembly_id
            && a.project_id         == b.project_id
            && a.part_id            == b.part_id
            && a.status             == b.status
            && a.quantity_planned   == b.quantity_planned
            && a.quantity_completed == b.quantity_completed
            && a.planned_start      == b.planned_start
            && a.planned_end        == b.planned_end
            && a.actual_start       == b.actual_start
            && a.actual_end         == b.actual_end
            && a.updated_at         == b.updated_at;
    }
    friend bool operator!=(const ProductionPartNode& a, const ProductionPartNode& b) { return !(a == b); }
};

/**
 * @brief Узел «сборка» в дереве производственного задания.
 *
 * Рекурсивно содержит дочерние сборки и список входящих деталей.
 * TODO(refactor/split): вынести в отдельный ресурс ProductionTaskAssemblyNode
 * c FK production_task_id и parent_assembly_node_id. См. TODO в ProductionPartNode.
 */
struct ProductionAssemblyNode {
    uint64_t assembly_id = 0;   // FK → assemblies.id
    uint64_t project_id  = 0;   // FK → projects.id

    TaskStatus status              = TaskStatus::PLANNED;
    uint64_t   quantity_planned    = 0;
    uint64_t   quantity_completed  = 0;

    QDateTime                planned_start;
    QDateTime                planned_end;
    std::optional<QDateTime> actual_start;
    std::optional<QDateTime> actual_end;

    // Дочерние узлы (рекурсивно)
    std::vector<ProductionAssemblyNode> child_assemblies;
    std::vector<ProductionPartNode>     parts;

    QDateTime updated_at;

    friend bool operator==(const ProductionAssemblyNode& a, const ProductionAssemblyNode& b) {
        return a.assembly_id        == b.assembly_id
            && a.project_id         == b.project_id
            && a.status             == b.status
            && a.quantity_planned   == b.quantity_planned
            && a.quantity_completed == b.quantity_completed
            && a.planned_start      == b.planned_start
            && a.planned_end        == b.planned_end
            && a.actual_start       == b.actual_start
            && a.actual_end         == b.actual_end
            && a.child_assemblies   == b.child_assemblies
            && a.parts              == b.parts
            && a.updated_at         == b.updated_at;
    }
    friend bool operator!=(const ProductionAssemblyNode& a, const ProductionAssemblyNode& b) { return !(a == b); }
};

} // namespace uniter::contract::production

#endif // PRODUCTIONTYPES_H
