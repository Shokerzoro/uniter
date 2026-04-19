#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "../resourceabstract.h"
#include "../documents/doclink.h"
#include "designtypes.h"

#include <tinyxml2.h>
#include <QString>
#include <cstdint>
#include <optional>
#include <vector>

namespace uniter::contract::design {

/**
 * @brief Ссылка на дочернюю сборку (связь N:M parent Assembly → child Assembly).
 *
 * Соответствует таблице `assembly_children` (см. docs/pdm_design_architecture.md §1.3).
 * Одна запись = одно вхождение дочерней сборки в родительскую с указанием
 * количества и исполнения (config).
 */
struct AssemblyChildRef {
    uint64_t child_assembly_id = 0;  // FK → assemblies.id
    uint32_t quantity          = 1;  // Количество вхождений
    QString  config;                 // Идентификатор исполнения ("01", "02", ... или пусто)
};

/**
 * @brief Ссылка на деталь в сборке (вхождение Part в Assembly).
 *
 * Хранится отдельной таблицей (планируется `assembly_parts` по тому же
 * принципу, что и `assembly_children`), но с FK на `parts.id`.
 */
struct AssemblyPartRef {
    uint64_t part_id  = 0;   // FK → parts.id
    uint32_t quantity = 1;
    QString  config;
};

/**
 * @brief Ресурс подсистемы DESIGN — сборка (узел дерева).
 *
 * Соответствует таблице `assemblies` (§1.3). Дочерние связи (сборка→сборка,
 * сборка→деталь) — в отдельных таблицах-ссылках, которые десериализуются в
 * `child_assemblies` / `parts`.
 *
 * Файлы (сборочный чертёж, спецификация, 3D-модель и т.п.) хранятся как
 * ресурсы подсистемы DOCUMENTS (Doc) и привязываются через DocLink.
 * Денормализованный список привязок копируется в `linked_documents` для
 * удобства UI; источник истины — таблица `doc_links`.
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
        std::optional<uint64_t> parent_assembly_id_,
        QString  designation_,
        QString  name_,
        QString  description_,
        AssemblyType type_ = AssemblyType::VIRTUAL)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , project_id         (project_id_)
        , parent_assembly_id (std::move(parent_assembly_id_))
        , designation        (std::move(designation_))
        , name               (std::move(name_))
        , description        (std::move(description_))
        , type               (type_) {}

    // Идентификация и иерархия
    uint64_t project_id = 0;                        // FK → projects.id
    std::optional<uint64_t> parent_assembly_id;     // FK → assemblies.id (NULL для корневой)

    // Атрибуты ЕСКД
    QString      designation;                       // Обозначение ЕСКД (например "СБ-001")
    QString      name;                              // Наименование
    QString      description;
    AssemblyType type = AssemblyType::VIRTUAL;      // REAL / VIRTUAL

    // Дочерние связи (десериализуются из таблиц-ссылок).
    std::vector<AssemblyChildRef> child_assemblies;
    std::vector<AssemblyPartRef>  parts;

    // Привязанные документы (денормализация таблицы doc_links по target_type=ASSEMBLY).
    // Сам DocLink — полноценный ресурс подсистемы DOCUMENTS; здесь хранится
    // копия для удобства сериализации и UI.
    std::vector<documents::DocLink> linked_documents;

    // Каскадная сериализация (базовые поля — через ResourceAbstract::to_xml)
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml  (tinyxml2::XMLElement* dest)   override;
};


} // namespace uniter::contract::design

#endif // ASSEMBLY_H
