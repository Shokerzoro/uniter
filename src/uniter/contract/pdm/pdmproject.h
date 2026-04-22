#ifndef PDMPROJECT_H
#define PDMPROJECT_H

#include "../resourceabstract.h"
#include <cstdint>
#include <optional>

namespace uniter::contract::pdm {

/**
 * @brief Ресурс подсистемы PDM — PDM-проект (версионная ветка).
 *
 * Соответствует таблице `pdm_project` (см. docs/db/pdm.md).
 *
 * PdmProject — контейнер снэпшотов одного design_project. Обратная связь
 * идёт со стороны DESIGN: `design_project.pdm_project_id → pdm_project.id`.
 *
 * На момент создания PdmProject оба поля `*_snapshot_id == NULL` (ветка
 * заведена, но ни одного снэпшота ещё нет). После первого парсинга
 * PDMManager создаёт Snapshot #1 и выставляет оба в его id.
 * Далее `head_snapshot_id` двигается при добавлении новых снэпшотов,
 * `base_snapshot_id` остаётся неизменным (корень истории).
 *
 * ResourceType::PDM_PROJECT = 52.
 */
class PdmProject : public ResourceAbstract {
public:
    PdmProject() = default;
    PdmProject(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        std::optional<uint64_t> base_snapshot_id_ = std::nullopt,
        std::optional<uint64_t> head_snapshot_id_ = std::nullopt)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , base_snapshot_id (std::move(base_snapshot_id_))
        , head_snapshot_id (std::move(head_snapshot_id_)) {}

    // FK на первый снэпшот ветки. NULL до первого парсинга.
    std::optional<uint64_t> base_snapshot_id;   // FK → pdm_snapshot.id

    // FK на актуальный (текущий) снэпшот ветки. NULL до первого парсинга.
    // После первого парсинга PDMManager выставляет его в id Snapshot #1,
    // далее двигает вперёд при добавлении новых снэпшотов.
    std::optional<uint64_t> head_snapshot_id;   // FK → pdm_snapshot.id

    friend bool operator==(const PdmProject& a, const PdmProject& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.base_snapshot_id == b.base_snapshot_id
            && a.head_snapshot_id == b.head_snapshot_id;
    }
    friend bool operator!=(const PdmProject& a, const PdmProject& b) { return !(a == b); }
};

} // namespace uniter::contract::pdm

#endif // PDMPROJECT_H
