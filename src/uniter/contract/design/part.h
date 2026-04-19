#ifndef PART_H
#define PART_H

#include "../resourceabstract.h"
#include "../documents/doclink.h"
#include "designtypes.h"

#include <tinyxml2.h>
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>
#include <vector>

namespace uniter::contract::design {

/**
 * @brief Ресурс подсистемы DESIGN — деталь (листовой узел дерева).
 *
 * Соответствует таблице `parts` (см. docs/pdm_design_architecture.md §1.4).
 * Исполнения и подписи — в отдельных таблицах `part_configs` / `part_signatures`,
 * десериализуются в `configs` / `signatures`.
 *
 * Связь с материалом — через FK `material_instance_id` → material_instances.id.
 * Внутри Part НЕ хранится сам MaterialInstance (никаких shared_ptr — только id),
 * это соответствует принципу ориентации на реляционную БД.
 *
 * Файлы (чертёж детали, 3D-модель) — ресурсы подсистемы DOCUMENTS (Doc);
 * привязки к Part живут в таблице `doc_links` и десериализуются в
 * `linked_documents` для удобства UI.
 */
class Part : public ResourceAbstract {
public:
    Part() = default;
    Part(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t project_id_,
        uint64_t assembly_id_,
        QString  designation_,
        QString  name_,
        QString  description_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , project_id  (project_id_)
        , assembly_id (assembly_id_)
        , designation (std::move(designation_))
        , name        (std::move(name_))
        , description (std::move(description_)) {}

    // Идентификация и иерархия
    uint64_t project_id  = 0;   // FK → projects.id
    uint64_t assembly_id = 0;   // FK → assemblies.id (родительская сборка)

    // Атрибуты ЕСКД
    QString designation;        // Обозначение ЕСКД (например "СБ-001-01")
    QString name;
    QString description;
    QString litera;             // Литера КД (О, О1, А, ...)
    QString organization;       // Организация-разработчик

    // Ссылка на материал (FK → material_instances.id).
    // nullopt для сборок, не имеющих материала как таковых (напр. изделия в сборе).
    std::optional<uint64_t> material_instance_id;

    // Исполнения детали (таблица `part_configs`)
    std::vector<PartConfig> configs;

    // Подписи на чертеже (таблица `part_signatures`)
    std::vector<PartSignature> signatures;

    // Привязанные документы (денормализация таблицы doc_links по target_type=PART).
    std::vector<documents::DocLink> linked_documents;

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml  (tinyxml2::XMLElement* dest)   override;

    friend bool operator==(const Part& a, const Part& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.project_id           == b.project_id
            && a.assembly_id          == b.assembly_id
            && a.designation          == b.designation
            && a.name                 == b.name
            && a.description          == b.description
            && a.litera               == b.litera
            && a.organization         == b.organization
            && a.material_instance_id == b.material_instance_id
            && a.configs              == b.configs
            && a.signatures           == b.signatures
            && a.linked_documents     == b.linked_documents;
    }
    friend bool operator!=(const Part& a, const Part& b) { return !(a == b); }
};


} // namespace uniter::contract::design

#endif // PART_H
