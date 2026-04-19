#ifndef DELTA_H
#define DELTA_H

#include "../resourceabstract.h"
#include "../documents/doctypes.h"   // DocumentType
#include "pdmtypes.h"

#include <tinyxml2.h>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <vector>

namespace uniter::contract::pdm {

/**
 * @brief Одно файловое изменение внутри DeltaChange.
 *
 * Соответствует таблице `delta_file_changes` (pdm_design_architecture.md §1.6).
 * old_* пусто при ADD, new_* пусто при DELETE.
 */
struct DeltaFileChange {
    uint64_t                id = 0;           // PK в delta_file_changes (server-side)
    documents::DocumentType file_type = documents::DocumentType::UNKNOWN;
    QString               old_object_key;
    QString               old_sha256;
    QString               new_object_key;
    QString               new_sha256;

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
 * Соответствует таблице `delta_changes` (pdm_design_architecture.md §1.6).
 * Связь с Part/Assembly — по `designation` (текстовая ссылка, а не FK):
 * это обеспечивает устойчивость к переименованиям id на сервере (см. §4).
 */
struct DeltaChange {
    uint64_t                     id = 0;            // PK в delta_changes (server-side)
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
 * Delta = «что изменилось» при переходе от базовой версии `snapshot_id`
 * к следующей `next_snapshot_id`. Позволяет отображать diff пользователю
 * и восстанавливать историю файла по designation (см. §4).
 *
 * Несмотря на вложенные структуры (DeltaChange, DeltaFileChange), в БД они
 * раскладываются в три таблицы: `deltas`, `delta_changes`, `delta_file_changes` —
 * это соответствует принципу ориентации на реляционную БД.
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
        uint64_t snapshot_id_,
        uint64_t next_snapshot_id_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , snapshot_id      (snapshot_id_)
        , next_snapshot_id (next_snapshot_id_) {}

    uint64_t snapshot_id      = 0;   // FK → snapshots.id (базовая версия, ОТ которой дельта)
    uint64_t next_snapshot_id = 0;   // FK → snapshots.id (версия, ДО которой дельта)

    // Содержательная часть дельты (раскладывается в delta_changes / delta_file_changes)
    std::vector<DeltaChange> changes;

    // Денормализованное поле для быстрого отображения в UI.
    // Обновляется при применении дельты: changes_count = changes.size().
    uint32_t changes_count = 0;

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml  (tinyxml2::XMLElement* dest)   override;

    friend bool operator==(const Delta& a, const Delta& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.snapshot_id      == b.snapshot_id
            && a.next_snapshot_id == b.next_snapshot_id
            && a.changes          == b.changes
            && a.changes_count    == b.changes_count;
    }
    friend bool operator!=(const Delta& a, const Delta& b) { return !(a == b); }
};


} // namespace uniter::contract::pdm

#endif // DELTA_H
