#ifndef DELTA_H
#define DELTA_H

#include "../resourceabstract.h"
#include "../documents/doctypes.h"   // DocumentType
#include "pdmtypes.h"
#include <QString>
#include <QStringList>
#include <cstdint>
#include <optional>
#include <vector>

namespace uniter::contract::pdm {

/**
 * @brief Одно файловое изменение внутри DeltaChange.
 *
 * Соответствует строке прикладной таблицы `pdm_delta_file_changes`
 * (детализация содержимого `changed_elements` — схема PDM.pdf помечает
 * этот слот "!!!", оставлена на стадию реализации PDMManager).
 *
 * old_* пусто при ADD, new_* пусто при DELETE.
 */
struct DeltaFileChange {
    uint64_t                id = 0;
    documents::DocumentType file_type = documents::DocumentType::UNKNOWN;
    QString                 old_object_key;
    QString                 old_sha256;
    QString                 new_object_key;
    QString                 new_sha256;

    friend bool operator==(const DeltaFileChange& a, const DeltaFileChange& b) {
        return a.id             == b.id
            && a.file_type      == b.file_type
            && a.old_object_key == b.old_object_key
            && a.old_sha256     == b.old_sha256
            && a.new_object_key == b.new_object_key
            && a.new_sha256     == b.new_sha256;
    }
    friend bool operator!=(const DeltaFileChange& a, const DeltaFileChange& b) { return !(a == b); }
};

/**
 * @brief Одно изменение внутри Delta — ADD/MODIFY/DELETE конкретного элемента.
 *
 * Соответствует строке прикладной таблицы `pdm_delta_changes`.
 * Связь с Part/Assembly — по `designation` (текстовая ссылка, не FK):
 * designation стабилен при переименовании id на сервере.
 */
struct DeltaChange {
    uint64_t                     id = 0;
    QString                      designation;       // Обозначение элемента
    DeltaElementType             element_type = DeltaElementType::PART;
    DeltaChangeType              change_type  = DeltaChangeType::MODIFY;
    QStringList                  changed_fields;    // напр. ["drawing","litera"]
    std::vector<DeltaFileChange> file_changes;

    friend bool operator==(const DeltaChange& a, const DeltaChange& b) {
        return a.id             == b.id
            && a.designation    == b.designation
            && a.element_type   == b.element_type
            && a.change_type    == b.change_type
            && a.changed_fields == b.changed_fields
            && a.file_changes   == b.file_changes;
    }
    friend bool operator!=(const DeltaChange& a, const DeltaChange& b) { return !(a == b); }
};

/**
 * @brief Ресурс подсистемы PDM — инкрементальные изменения между снапшотами.
 *
 * Соответствует таблице `pdm_delta` (см. docs/db/pdm.md).
 *
 * Delta ссылается одновременно на пару смежных снэпшотов через
 * prev/next_snapshot_id и на пару смежных дельт через prev/next_delta_id —
 * это двусторонний связный список в пределах pdm_project. При этом
 * parent-связь с каким-то одним снэпшотом отсутствует: дельта «висит» на
 * ребре между prev_snapshot и next_snapshot.
 *
 * Несмотря на вложенные структуры (DeltaChange, DeltaFileChange), в БД
 * они раскладываются в три прикладные таблицы: `pdm_delta`,
 * `pdm_delta_changes`, `pdm_delta_file_changes` — это соответствует
 * принципу ориентации на реляционную БД.
 *
 * TODO(на подумать): структурная схема помечает содержимое дельты как
 * `changed_elements "!!!"` — формат не зафиксирован. Альтернатива
 * трём таблицам — JSON-поле внутри pdm_delta. Остановиться на одном
 * варианте при реализации PDMManager.
 */
class Delta : public ResourceAbstract {
public:
    Delta() = default;
    Delta(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        std::optional<uint64_t> prev_snapshot_id_ = std::nullopt,
        std::optional<uint64_t> next_snapshot_id_ = std::nullopt,
        std::optional<uint64_t> prev_delta_id_    = std::nullopt,
        std::optional<uint64_t> next_delta_id_    = std::nullopt)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , prev_snapshot_id (std::move(prev_snapshot_id_))
        , next_snapshot_id (std::move(next_snapshot_id_))
        , prev_delta_id    (std::move(prev_delta_id_))
        , next_delta_id    (std::move(next_delta_id_)) {}

    // Связь со снэпшотами, между которыми лежит дельта.
    std::optional<uint64_t> prev_snapshot_id;   // FK → pdm_snapshot.id (ОТ)
    std::optional<uint64_t> next_snapshot_id;   // FK → pdm_snapshot.id (ДО)

    // Связь с соседними дельтами — двусвязный список дельт проекта.
    std::optional<uint64_t> prev_delta_id;      // FK → pdm_delta.id (слева)
    std::optional<uint64_t> next_delta_id;      // FK → pdm_delta.id (справа)

    // Содержательная часть дельты (раскладывается в pdm_delta_changes /
    // pdm_delta_file_changes). В PDF-схеме — столбец `changed_elements "!!!"`.
    std::vector<DeltaChange> changes;

    // Денормализованное поле для быстрого отображения в UI.
    // Обновляется при применении дельты: changes_count = changes.size().
    uint32_t changes_count = 0;

    friend bool operator==(const Delta& a, const Delta& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.prev_snapshot_id == b.prev_snapshot_id
            && a.next_snapshot_id == b.next_snapshot_id
            && a.prev_delta_id    == b.prev_delta_id
            && a.next_delta_id    == b.next_delta_id
            && a.changes          == b.changes
            && a.changes_count    == b.changes_count;
    }
    friend bool operator!=(const Delta& a, const Delta& b) { return !(a == b); }
};

} // namespace uniter::contract::pdm

#endif // DELTA_H
