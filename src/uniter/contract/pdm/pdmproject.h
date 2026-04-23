#ifndef PDMPROJECT_H
#define PDMPROJECT_H

#include "../resourceabstract.h"
#include <cstdint>
#include <memory>

namespace uniter::contract::pdm {

class Snapshot; // fwd

/**
 * @brief Ресурс подсистемы PDM — PDM-проект (версионная ветка).
 *
 * Соответствует таблице `pdm_project` (см. docs/db/pdm.md).
 *
 * PdmProject — контейнер снэпшотов одного design_project. Обратная связь
 * идёт со стороны DESIGN: `design_project.pdm_project_id → pdm_project.id`.
 *
 * На момент создания PdmProject оба указателя `*_snapshot == nullptr`
 * (ветка заведена, но ни одного снэпшота ещё нет). После первого
 * парсинга PDMManager создаёт Snapshot #1 и выставляет оба на него.
 * Далее `head_snapshot` двигается при добавлении новых снэпшотов,
 * `base_snapshot` остаётся неизменным (корень истории).
 *
 * **Связь указателями.**
 * В БД обе связи — FK по id → pdm_snapshot.id. В рантайм-классе
 * используются `std::shared_ptr<Snapshot>`, чтобы PDMManager мог
 * переходить от PdmProject к началу/концу цепочки и далее по next_snapshot
 * без дополнительных запросов. Сам id каждого снэпшота по-прежнему
 * доступен через `Snapshot::id`.
 *
 * ResourceType::PDM_PROJECT = 52.
 */
class PdmProject : public ResourceAbstract {
public:
    PdmProject()
        : ResourceAbstract(
              Subsystem::PDM,
              GenSubsystemType::NOTGEN,
              ResourceType::PDM_PROJECT) {}
    PdmProject(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        std::shared_ptr<Snapshot> base_snapshot_ = nullptr,
        std::shared_ptr<Snapshot> head_snapshot_ = nullptr)
        : ResourceAbstract(
              Subsystem::PDM,
              GenSubsystemType::NOTGEN,
              ResourceType::PDM_PROJECT,
              s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , base_snapshot (std::move(base_snapshot_))
        , head_snapshot (std::move(head_snapshot_)) {}

    // Указатель на первый снэпшот ветки. nullptr до первого парсинга.
    std::shared_ptr<Snapshot> base_snapshot;

    // Указатель на актуальный (текущий) снэпшот ветки. nullptr до первого
    // парсинга. После первого парсинга PDMManager выставляет сюда Snapshot #1,
    // далее двигает вперёд при добавлении новых снэпшотов.
    std::shared_ptr<Snapshot> head_snapshot;

    friend bool operator==(const PdmProject& a, const PdmProject& b) {
        // Указатели сравниваем по адресу: равенство ветки определяется
        // тем, что она ссылается на одни и те же Snapshot-объекты.
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.base_snapshot.get() == b.base_snapshot.get()
            && a.head_snapshot.get() == b.head_snapshot.get();
    }
    friend bool operator!=(const PdmProject& a, const PdmProject& b) { return !(a == b); }
};

} // namespace uniter::contract::pdm

#endif // PDMPROJECT_H
