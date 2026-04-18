#include "assembly.h"
#include <tinyxml2.h>

namespace uniter::contract::design {


void Assembly::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    // Каскадная сериализация базовых полей
    ResourceAbstract::to_xml(dest);

    // Скалярные поля Assembly
    putUInt64   (dest, "project_id",         project_id);
    putOptUInt64(dest, "parent_assembly_id", parent_assembly_id);
    putString   (dest, "designation",        designation);
    putString   (dest, "name",               name);
    putString   (dest, "description",        description);
    putInt      (dest, "type",               static_cast<int>(type));

    putString   (dest, "drawing_object_key",          drawing_object_key);
    putString   (dest, "drawing_sha256",              drawing_sha256);
    putString   (dest, "spec_object_key",             spec_object_key);
    putString   (dest, "spec_sha256",                 spec_sha256);
    putString   (dest, "mounting_drawing_object_key", mounting_drawing_object_key);
    putString   (dest, "mounting_drawing_sha256",     mounting_drawing_sha256);
    putString   (dest, "model_3d_object_key",         model_3d_object_key);
    putString   (dest, "model_3d_sha256",             model_3d_sha256);

    // Дочерние связи — вложенными элементами
    auto* doc = dest->GetDocument();
    if (!doc) return;

    tinyxml2::XMLElement* childrenEl = doc->NewElement("ChildAssemblies");
    for (const auto& ref : child_assemblies) {
        tinyxml2::XMLElement* r = doc->NewElement("AssemblyChildRef");
        putUInt64(r, "child_assembly_id", ref.child_assembly_id);
        putUInt32(r, "quantity",          ref.quantity);
        putString(r, "config",            ref.config);
        childrenEl->InsertEndChild(r);
    }
    dest->InsertEndChild(childrenEl);

    tinyxml2::XMLElement* partsEl = doc->NewElement("Parts");
    for (const auto& ref : parts) {
        tinyxml2::XMLElement* r = doc->NewElement("AssemblyPartRef");
        putUInt64(r, "part_id",  ref.part_id);
        putUInt32(r, "quantity", ref.quantity);
        putString(r, "config",   ref.config);
        partsEl->InsertEndChild(r);
    }
    dest->InsertEndChild(partsEl);
}

void Assembly::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    project_id         = getUInt64   (source, "project_id");
    parent_assembly_id = getOptUInt64(source, "parent_assembly_id");
    designation        = getString   (source, "designation");
    name               = getString   (source, "name");
    description        = getString   (source, "description");
    type               = static_cast<AssemblyType>(getInt(source, "type",
                                                          static_cast<int>(AssemblyType::VIRTUAL)));

    drawing_object_key          = getString(source, "drawing_object_key");
    drawing_sha256              = getString(source, "drawing_sha256");
    spec_object_key             = getString(source, "spec_object_key");
    spec_sha256                 = getString(source, "spec_sha256");
    mounting_drawing_object_key = getString(source, "mounting_drawing_object_key");
    mounting_drawing_sha256     = getString(source, "mounting_drawing_sha256");
    model_3d_object_key         = getString(source, "model_3d_object_key");
    model_3d_sha256             = getString(source, "model_3d_sha256");

    child_assemblies.clear();
    if (auto* childrenEl = source->FirstChildElement("ChildAssemblies")) {
        for (auto* r = childrenEl->FirstChildElement("AssemblyChildRef");
             r; r = r->NextSiblingElement("AssemblyChildRef")) {
            AssemblyChildRef ref;
            ref.child_assembly_id = getUInt64(r, "child_assembly_id");
            ref.quantity          = getUInt32(r, "quantity", 1);
            ref.config            = getString(r, "config");
            child_assemblies.push_back(std::move(ref));
        }
    }

    parts.clear();
    if (auto* partsEl = source->FirstChildElement("Parts")) {
        for (auto* r = partsEl->FirstChildElement("AssemblyPartRef");
             r; r = r->NextSiblingElement("AssemblyPartRef")) {
            AssemblyPartRef ref;
            ref.part_id  = getUInt64(r, "part_id");
            ref.quantity = getUInt32(r, "quantity", 1);
            ref.config   = getString(r, "config");
            parts.push_back(std::move(ref));
        }
    }
}


} // namespace uniter::contract::design
