#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "../resourceabstract.h"
#include "pdmtypes.h"
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>

namespace uniter::contract::pdm {

/**
 * @brief Ресурс подсистемы PDM — зафиксированный срез проекта.
 *
 * Соответствует таблице `pdm_snapshot` (см. docs/db/pdm.md).
 *
 * Snapshot — неизменяемый срез DESIGN в определённый момент времени.
 * Snapshot'ы связаны в двусвязный список через prev/next_snapshot_id,
 * при этом каждая пара соседних снэпшотов дополнительно связана
 * ресурсом Delta (prev/next_delta_id дублируют связь для быстрой
 * навигации в обе стороны без JOIN'ов).
 *
 * root_assembly_id указывает на `pdm_assembly.id` — ЗАМОРОЖЕННУЮ КОПИЮ
 * корневой сборки (не на design_assembly). Это гарантирует, что любые
 * правки в design_* не нарушат консистентность снэпшота.
 *
 * Механизм создания snapshot:
 *   1. PDMManager парсит PDF проекта.
 *   2. Создаёт новый pdm_snapshot.
 *   3. Копирует все актуальные design_assembly/design_part/design_assembly_config/
 *      design_part_config (и все join-таблицы) в соответствующие pdm_* таблицы,
 *      проставляя им FK snapshot_id.
 *   4. Выставляет pdm_snapshot.root_assembly_id на id замороженного pdm_assembly.
 *   5. Обновляет pdm_project.head_snapshot_id на новый snapshot.
 *   6. Создаёт pdm_delta между предыдущим head и новым head, связывает
 *      prev/next_delta_id у обоих снэпшотов.
 */
class Snapshot : public ResourceAbstract {
public:
    Snapshot() = default;
    Snapshot(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t root_assembly_id_,
        std::optional<uint64_t> prev_snapshot_id_ = std::nullopt,
        std::optional<uint64_t> next_snapshot_id_ = std::nullopt,
        std::optional<uint64_t> prev_delta_id_    = std::nullopt,
        std::optional<uint64_t> next_delta_id_    = std::nullopt)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , root_assembly_id (root_assembly_id_)
        , prev_snapshot_id (std::move(prev_snapshot_id_))
        , next_snapshot_id (std::move(next_snapshot_id_))
        , prev_delta_id    (std::move(prev_delta_id_))
        , next_delta_id    (std::move(next_delta_id_)) {}

    // Корневая сборка снэпшота — ссылка на ЗАМОРОЖЕННУЮ копию (pdm_assembly),
    // не на design_assembly. Это инвариант неизменности снэпшота.
    uint64_t root_assembly_id = 0;    // FK → pdm_assembly.id

    // Двусторонний связный список снэпшотов (версионная цепочка).
    std::optional<uint64_t> prev_snapshot_id;   // FK → pdm_snapshot.id
    std::optional<uint64_t> next_snapshot_id;   // FK → pdm_snapshot.id

    // Дублирующие FK на смежные дельты — для быстрой навигации.
    // prev_delta_id = дельта, приведшая в этот снэпшот (от prev_snapshot);
    // next_delta_id = дельта, уходящая из этого снэпшота (в next_snapshot).
    std::optional<uint64_t> prev_delta_id;      // FK → pdm_delta.id
    std::optional<uint64_t> next_delta_id;      // FK → pdm_delta.id

    // ----------------------------------------------------------------------
    // Прикладные поля (TODO: подтвердить необходимость при реализации
    // PDMManager). Структурная схема PDM.pdf их не требует, но рабочий
    // цикл снэпшота (DRAFT → APPROVED → ARCHIVED) и отчёты о валидации
    // ЕСКД без них неудобны. Оставлены как TODO на подумать.
    // ----------------------------------------------------------------------

    // Версия снэпшота в пределах pdm_project. Монотонно растёт.
    uint32_t version = 0;

    // Жизненный цикл (DRAFT/APPROVED/ARCHIVED).
    SnapshotStatus status = SnapshotStatus::DRAFT;

    // Метаданные утверждения — заполняются при переходе DRAFT → APPROVED
    std::optional<uint64_t>  approved_by;
    std::optional<QDateTime> approved_at;

    // Итоги валидации ЕСКД (денормализация pdm_snapshot_errors, которую
    // схема помечает "!!!" — детализация откладывается).
    uint32_t error_count   = 0;
    uint32_t warning_count = 0;

    friend bool operator==(const Snapshot& a, const Snapshot& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.root_assembly_id == b.root_assembly_id
            && a.prev_snapshot_id == b.prev_snapshot_id
            && a.next_snapshot_id == b.next_snapshot_id
            && a.prev_delta_id    == b.prev_delta_id
            && a.next_delta_id    == b.next_delta_id
            && a.version          == b.version
            && a.status           == b.status
            && a.approved_by      == b.approved_by
            && a.approved_at      == b.approved_at
            && a.error_count      == b.error_count
            && a.warning_count    == b.warning_count;
    }
    friend bool operator!=(const Snapshot& a, const Snapshot& b) { return !(a == b); }
};

} // namespace uniter::contract::pdm

#endif // SNAPSHOT_H
