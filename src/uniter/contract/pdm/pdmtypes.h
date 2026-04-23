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
 * @brief Тип элемента Delta — используется ключом std::map<Key, pair<old,new>>.
 *
 * Типы соответствуют ResourceType'ам, которые могут участвовать в дельте
 * проекта: сборка, исполнение сборки, деталь, исполнение детали. Для
 * бизнес-логики применения дельт атомарная единица изменения — один из
 * этих четырёх объектов.
 */
enum class DeltaElementType : uint8_t {
    ASSEMBLY        = 0,
    ASSEMBLY_CONFIG = 1,
    PART            = 2,
    PART_CONFIG     = 3
};

/**
 * @brief Уровень серьёзности диагностики парсинга.
 *
 * ERROR   — блокирующая проблема (нет файла, нет спецификации) — snapshot
 *           остаётся DRAFT до ручного исправления.
 * WARNING — некритичная проблема (неформальное изменение имени, пропущенная
 *           подпись) — snapshot может быть APPROVED.
 * INFO    — информационное сообщение (пропущен рекомендованный атрибут).
 */
enum class DiagnosticSeverity : uint8_t {
    ERROR   = 0,
    WARNING = 1,
    INFO    = 2
};

/**
 * @brief Крупная категория диагностики парсинга.
 *
 * FILE_SYSTEM     — проблемы с файлами на диске (нет файла, нет прав).
 * VERSION_CONTROL — проблемы жизненного цикла версий (неформальное изменение).
 * PARSING         — проблемы разбора PDF/структуры документа.
 * VALIDATION      — проблемы соответствия ЕСКД (отсутствуют обязательные поля).
 * OTHER           — прочее / неклассифицированное.
 *
 * Код конкретной проблемы (NO_FILE, INFORMAL_CHANGE, …) живёт в
 * `Diagnostic::type` как строка; перечень канонических значений —
 * в docs/db/pdm.md.
 */
enum class DiagnosticCategory : uint8_t {
    FILE_SYSTEM     = 0,
    VERSION_CONTROL = 1,
    PARSING         = 2,
    VALIDATION      = 3,
    OTHER           = 255
};

} // namespace uniter::contract::pdm

#endif // PDMTYPES_H
