#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "../resourceabstract.h"
#include "../documents/doclink.h"
#include "designtypes.h"
#include <QString>
#include <cstdint>
#include <vector>

namespace uniter::contract::design {

/**
 * @brief Ресурс подсистемы DESIGN — сборка (общие данные изделия).
 *
 * Соответствует таблице `design_assembly` (см. docs/db/design.md).
 *
 * Сборка хранит ТОЛЬКО общие данные, не зависящие от исполнения:
 * обозначение, наименование, описание, тип. Структура сборки (состав
 * подсборок, деталей, стандартных изделий, материалов) вынесена на
 * уровень исполнения — см. `AssemblyConfig` (`design_assembly_config` +
 * join-таблицы).
 *
 * Родительство Assembly→Assembly в таблице НЕ хранится: одна сборка
 * может входить в состав нескольких разных сборок через
 * `design_assembly_config_children`.
 *
 * Файлы (сборочный чертёж, спецификация, 3D-модель) — ресурсы
 * подсистемы DOCUMENTS (Doc); привязки живут в таблице `documents_doc_link`
 * с `target_type = ASSEMBLY`. Денормализованная копия — в `linked_documents`
 * (для удобства UI; источник истины всё равно doc_links).
 */
class Assembly : public ResourceAbstract {
public:
    Assembly() = default;
    Assembly(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t project_id_,
        QString  designation_,
        QString  name_,
        QString  description_,
        AssemblyType type_ = AssemblyType::VIRTUAL)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , project_id  (project_id_)
        , designation (std::move(designation_))
        , name        (std::move(name_))
        , description (std::move(description_))
        , type        (type_) {}

    // FK на проект, к которому сборка относится.
    uint64_t project_id = 0;                       // FK → design_project.id

    // Атрибуты ЕСКД
    QString      designation;                      // Обозначение ЕСКД (например "СБ-001")
    QString      name;                             // Наименование
    QString      description;
    AssemblyType type = AssemblyType::VIRTUAL;     // REAL / VIRTUAL

    // Привязанные документы (денормализация doc_links по target_type=ASSEMBLY).
    // Сам DocLink — полноценный ресурс DOCUMENTS; здесь хранится копия
    // для удобства отображения в UI.
    std::vector<documents::DocLink> linked_documents;

    // Отсутствуют (перенесены в AssemblyConfig):
    //   - parent_assembly_id  → в join design_assembly_config_children
    //   - child_assemblies[]  → в join design_assembly_config_children
    //   - parts[]             → в join design_assembly_config_parts
    // Ассоциации с исполнениями (configs[]) тоже хранятся снаружи:
    // один Assembly имеет N AssemblyConfig через FK `assembly_id` у конфига.

    friend bool operator==(const Assembly& a, const Assembly& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.project_id       == b.project_id
            && a.designation      == b.designation
            && a.name             == b.name
            && a.description      == b.description
            && a.type             == b.type
            && a.linked_documents == b.linked_documents;
    }
    friend bool operator!=(const Assembly& a, const Assembly& b) { return !(a == b); }
};

} // namespace uniter::contract::design

#endif // ASSEMBLY_H
