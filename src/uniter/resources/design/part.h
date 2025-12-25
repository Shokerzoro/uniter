#ifndef PART_H
#define PART_H

#include "../resourceabstract.h"
#include "../materialinstance/materialinstancebase.h"
#include "fileversion.h"
#include <tinyxml2.h>
#include <QString>
#include <vector>
#include <cstdint>
#include <memory>

namespace uniter::resources::design {

class Part : public ResourceAbstract
{
public:
    Part(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString name_,
        QString description_,
        uint64_t source_project_id_,
        uint64_t assembly_id_,
        uint64_t part_id_,
        std::vector<std::shared_ptr<FileVersion>> part_drawings_,
        std::vector<std::shared_ptr<FileVersion>> models_3d_,
        std::shared_ptr<MaterialInstanceBase> material_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
        name(std::move(name_)),
        description(std::move(description_)),
        source_project_id(source_project_id_),
        assembly_id(assembly_id_),
        part_id(part_id_),
        part_drawings(std::move(part_drawings_)),
        models_3d(std::move(models_3d_)),
        material(material_) {}

    // Специфичные поля Part
    QString name;
    QString description;
    uint64_t source_project_id;
    uint64_t assembly_id;
    uint64_t part_id;

    // Документы
    std::vector<std::shared_ptr<FileVersion>> part_drawings;
    std::vector<std::shared_ptr<FileVersion>> models_3d;
    std::shared_ptr<MaterialInstanceBase> material;

    // Сериалиализация десериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


class PartReference {
public:
    PartReference(
        uint64_t source_project_id_,
        uint64_t assembly_id_,
        uint64_t part_id_,
        std::shared_ptr<Part> part_,
        uint64_t quantity_)
        : source_project_id(source_project_id_),
        assembly_id(assembly_id_),
        part_id(part_id_),
        part(part_),
        quantity(quantity_) {}

    uint64_t source_project_id;
    uint64_t assembly_id;
    uint64_t part_id;
    std::shared_ptr<Part> part;
    uint64_t quantity;
};


} // design

#endif // PART_H
