#ifndef PART_H
#define PART_H

#include "../materialinstance/materialinstancebase.h"
#include "fileversion.h"
#include <tinyxml2.h>
#include <QString>
#include <vector>
#include <cstdint>
#include <memory>

namespace uniter::resources::design {

class PartReference;

class Part {
public:
    Part(
        QString name_,
        QString description_,
        uint64_t source_project_id_,
        uint64_t assembly_id_,
        uint64_t part_id_,
        uint64_t s_created_by_,
        uint64_t s_updated_by_,
        bool actual_,
        std::vector<std::shared_ptr<FileVersion>> part_drawings_,
        std::vector<std::shared_ptr<FileVersion>> models_3d_,
        std::shared_ptr<MaterialInstanceBase> material_)
        : name(std::move(name_)),
        description(std::move(description_)),
        source_project_id(source_project_id_),
        assembly_id(assembly_id_),
        part_id(part_id_),
        s_created_by(s_created_by_),
        s_updated_by(s_updated_by_),
        actual(actual_),
        part_drawings(std::move(part_drawings_)),
        models_3d(std::move(models_3d_)),
        material(material_) {}

    QString name;
    QString description;
    uint64_t source_project_id;
    uint64_t assembly_id;
    uint64_t part_id;
    uint64_t s_created_by;
    uint64_t s_updated_by;
    bool actual;

    // Документы
    std::vector<std::shared_ptr<FileVersion>> part_drawings;
    std::vector<std::shared_ptr<FileVersion>> models_3d;
    std::shared_ptr<MaterialInstanceBase> material;

    void from_xml(tinyxml2::XMLElement* source);
    void to_xml(tinyxml2::XMLElement* dest);
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
