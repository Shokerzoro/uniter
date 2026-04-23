#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "../resourceabstract.h"
#include "../documents/doclink.h"
#include "designtypes.h"
#include "assemblyconfig.h"
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
 * обозначение, наименование, описание, тип. Состав сборки (подсборки,
 * детали, стандартные изделия, материалы) хранится в её исполнениях —
 * `AssemblyConfig` (`design_assembly_config` + join-таблицы).
 *
 * Родительство Assembly→Assembly в таблице НЕ хранится: одна сборка
 * может входить в состав нескольких разных сборок через
 * `design_assembly_config_children`.
 *
 * **Runtime-сворачивание таблицы связей в класс.**
 * В БД связь `Assembly 1:M AssemblyConfig` выражена через FK
 * `design_assembly_config.assembly_id`. В рантайме у класса `Assembly`
 * есть поле `configs` — вектор `AssemblyConfig`, который материализуется
 * DataManagerом при загрузке сборки. Это позволяет коду напрямую
 * работать с конфигурациями сборки (итерировать, искать по коду и т.п.)
 * без дополнительного запроса. CRUD по отдельному `AssemblyConfig`
 * продолжает идти через UniterMessage с `ResourceType::ASSEMBLY_CONFIG`
 * (это отдельный ресурс со своим id).
 *
 * Файлы (сборочный чертёж, спецификация, 3D-модель) — ресурсы
 * подсистемы DOCUMENTS (Doc); привязки живут в таблице `documents_doc_link`
 * с `target_type = ASSEMBLY`. Денормализованная копия — в `linked_documents`
 * (для удобства UI; источник истины всё равно doc_links).
 */
class Assembly : public ResourceAbstract {
public:
    Assembly()
        : ResourceAbstract(
              Subsystem::DESIGN,
              GenSubsystemType::NOTGEN,
              ResourceType::ASSEMBLY) {}
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
        : ResourceAbstract(
              Subsystem::DESIGN,
              GenSubsystemType::NOTGEN,
              ResourceType::ASSEMBLY,
              s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
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

    // Исполнения сборки (runtime-сворачивание связи 1:M из БД).
    // В БД — отдельная таблица `design_assembly_config` с FK `assembly_id`;
    // здесь DataManager материализует все актуальные исполнения данной
    // сборки в вектор, чтобы бизнес-логика и UI могли работать с полным
    // набором исполнений без дополнительных запросов.
    //
    // CRUD по отдельному исполнению всё равно идёт через
    // ResourceType::ASSEMBLY_CONFIG (у AssemblyConfig свой id); этот
    // вектор обновляется Observerом DataManagerа при каждом изменении
    // дочернего конфига.
    std::vector<AssemblyConfig> configs;

    // Привязанные документы (денормализация doc_links по target_type=ASSEMBLY).
    // Сам DocLink — полноценный ресурс DOCUMENTS; здесь хранится копия
    // для удобства отображения в UI.
    std::vector<documents::DocLink> linked_documents;

    // Отсутствуют (перенесены в AssemblyConfig):
    //   - parent_assembly_id  → в join design_assembly_config_children
    //   - child_assemblies[]  → в join design_assembly_config_children
    //   - parts[]             → в join design_assembly_config_parts

    friend bool operator==(const Assembly& a, const Assembly& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.project_id       == b.project_id
            && a.designation      == b.designation
            && a.name             == b.name
            && a.description      == b.description
            && a.type             == b.type
            && a.configs          == b.configs
            && a.linked_documents == b.linked_documents;
    }
    friend bool operator!=(const Assembly& a, const Assembly& b) { return !(a == b); }
};

} // namespace uniter::contract::design

#endif // ASSEMBLY_H
