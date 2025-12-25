#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "../materialinstance/materialinstancebase.h"
#include "../resourceabstract.h"
#include "fileversion.h"
#include "part.h"

#include <tinyxml2.h>
#include <QString>
#include <optional>
#include <vector>
#include <map>

namespace uniter::resources::design {

class AssemblyReference;

enum class AssemblyType : uint8_t {
    REAL    = 0,      // Реальная сборка (найдена спецификация)
    VIRTUAL = 1    // Виртуальная сборка (спецификация не обнаружена)
};

class Assembly : public ResourceAbstract {
public:
    Assembly(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t project_id_,
        uint64_t assembly_id_,
        uint64_t part_id_ctr_,
        QString name_,
        QString description_,
        AssemblyType type_,
        std::optional<uint64_t> parent_assembly_id_,
        std::map<uint64_t, AssemblyReference> child_assemblies_,
        std::map<uint64_t, PartReference> parts_,
        std::vector<MaterialInstanceBase> auxiliary_materials_,
        std::shared_ptr<FileVersion> assembly_drawings_,
        std::shared_ptr<FileVersion> mounting_drawings_,
        std::shared_ptr<FileVersion> models_3d_,
        std::shared_ptr<FileVersion> specifications_
        ) : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
        project_id(std::move(project_id_)),
        assembly_id(std::move(assembly_id_)),
        part_id_ctr(std::move(part_id_ctr_)),
        name(std::move(name_)),
        description(std::move(description_)),
        type(std::move(type_)),
        parent_assembly_id(std::move(parent_assembly_id_)),
        child_assemblies(std::move(child_assemblies_)),
        parts(std::move(parts_)),
        auxiliary_materials(std::move(auxiliary_materials_)),
        assembly_drawings(std::move(assembly_drawings_)),
        mounting_drawings(std::move(mounting_drawings_)),
        models_3d(std::move(models_3d_)),
        specifications(std::move(specifications_)) {}

    uint64_t project_id;
    uint64_t assembly_id;
    uint64_t part_id_ctr;
    QString name;
    QString description;

    // Тип сборки (реальная/виртуальная)
    AssemblyType type = AssemblyType::VIRTUAL;
    std::optional<uint64_t> parent_assembly_id;

    // Локальные дочерние сборки (map: local_id -> reference)
    std::map<uint64_t, AssemblyReference> child_assemblies;
    std::map<uint64_t, PartReference> parts;
    // Вспомогательные материалы
    std::vector<MaterialInstanceBase> auxiliary_materials;

    // Документы
    std::shared_ptr<FileVersion> assembly_drawings;
    std::shared_ptr<FileVersion> mounting_drawings;
    std::shared_ptr<FileVersion> models_3d;
    std::shared_ptr<FileVersion> specifications;

    // Сериализация десериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


class AssemblyReference {
    AssemblyReference(
        uint64_t source_project_id_,
        uint64_t assembly_id_,
        std::shared_ptr<Assembly> assembly_,
        uint64_t quantity_)
        : source_project_id(source_project_id_),
        assembly_id(assembly_id_),
        assembly(assembly_),
        quantity(quantity_) {}

    uint64_t source_project_id;
    uint64_t assembly_id;     // local_id в источнике
    std::shared_ptr<Assembly> assembly;
    uint64_t quantity;
};



} // design

#endif // ASSEMBLY_H
