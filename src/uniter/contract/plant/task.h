#ifndef TASK_H
#define TASK_H

#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>
#include <vector>
#include <map>

namespace uniter::contract::plant {

/**
 * @brief Статус производственного задания.
 *
 * Состояния задания и его дочерних узлов (ProductionAssemblyNode,
 * ProductionPartNode) приводятся к общему enum, т.к. для аналитики ERP
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
 * Не самостоятельный ресурс — входит в состав Task. Хранит количественные
 * показатели по конкретной детали конкретной сборки проекта.
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
 * Не самостоятельный ресурс — входит в состав Task. Может рекурсивно
 * содержать дочерние сборки и список входящих деталей.
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


/**
 * @brief Производственное задание (ресурс подсистемы PRODUCTION).
 *
 * Ссылается на проект через `project_id` и на зафиксированный Snapshot
 * через `snapshot_id` (pdm_design_architecture.md §3): если после создания
 * задания проект получит новый APPROVED Snapshot, задание продолжит
 * ссылаться на снэпшот, действовавший на момент создания.
 */
class Task : public ResourceAbstract
{
public:
    Task() = default;
    Task(
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
          quantity(quantity_),
          status(status_),
          plant_id(plant_id_) {}

    // Идентификация задания
    QString  code;
    QString  name;

    // Связь с конструкторской подсистемой
    uint64_t project_id  = 0;   // FK → projects.id
    uint64_t snapshot_id = 0;   // FK → snapshots.id (зафиксированный состав)
    uint64_t plant_id    = 0;   // FK → plants.id (площадка выполнения)

    // Количественные параметры
    uint64_t   quantity = 0;
    TaskStatus status   = TaskStatus::PLANNED;

    // Дерево производственных узлов (сборки + детали)
    std::vector<ProductionAssemblyNode> root_assemblies;

    // Период выполнения
    QDateTime                planned_start;
    QDateTime                planned_end;
    std::optional<QDateTime> actual_start;
    std::optional<QDateTime> actual_end;

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;

    friend bool operator==(const Task& a, const Task& b) {
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
    friend bool operator!=(const Task& a, const Task& b) { return !(a == b); }
};


} // namespace uniter::contract::plant

#endif // TASK_H
