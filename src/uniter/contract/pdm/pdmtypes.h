#ifndef PDMTYPES_H
#define PDMTYPES_H

#include <cstdint>

namespace uniter::contract::pdm {

/**
 * @brief Жизненный цикл Snapshot.
 *
 * DRAFT    — создан PDMManager после парсинга, виден только PDM-менеджерам.
 * APPROVED — утверждён ответственным (рабочий состав для других подсистем).
 * ARCHIVED — заменён последующим APPROVED.
 *
 * Связь с DESIGN идёт через `design_project.pdm_project_id →
 * pdm_project.head_snapshot_id` (см. docs/db/pdm_design_logic.md §5).
 * Вопрос "head vs approved" отложен до реализации PDMManager: нужно ли
 * выделять `pdm_project.approved_snapshot_id` рядом с head_snapshot_id —
 * TODO.
 *
 * Snapshot никогда не удаляется — только архивируется, это обеспечивает
 * воспроизводимость Task'ов, ссылающихся на него.
 */
enum class SnapshotStatus : uint8_t {
    DRAFT    = 0,
    APPROVED = 1,
    ARCHIVED = 2
};

/**
 * @brief Тип элемента, к которому относится изменение в Delta.
 */
enum class DeltaElementType : uint8_t {
    PART     = 0,
    ASSEMBLY = 1
};

/**
 * @brief Тип изменения в Delta.
 *
 * ADD    — элемент появился в новой версии (new_object_key заполнены, old — пусты).
 * MODIFY — элемент существовал, но изменились его поля/файлы.
 * DELETE — элемент удалён в новой версии (old_object_key заполнены, new — пусты).
 */
enum class DeltaChangeType : uint8_t {
    ADD    = 0,
    MODIFY = 1,
    DELETE = 2
};

} // namespace uniter::contract::pdm

#endif // PDMTYPES_H
