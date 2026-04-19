
#include "part.h"
#include <tinyxml2.h>

namespace uniter::contract::design {


void Part::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    // Скалярные поля
    putUInt64   (dest, "project_id",           project_id);
    putUInt64   (dest, "assembly_id",          assembly_id);
    putString   (dest, "designation",          designation);
    putString   (dest, "name",                 name);
    putString   (dest, "description",          description);
    putString   (dest, "litera",               litera);
    putString   (dest, "organization",         organization);
    putOptUInt64(dest, "material_instance_id", material_instance_id);

    auto* doc = dest->GetDocument();
    if (!doc) return;

    // Исполнения
    tinyxml2::XMLElement* configsEl = doc->NewElement("Configs");
    for (const auto& cfg : configs) {
        tinyxml2::XMLElement* c = doc->NewElement("Config");
        putString(c, "config_id", cfg.config_id);
        putDouble(c, "length_mm", cfg.length_mm);
        putDouble(c, "width_mm",  cfg.width_mm);
        putDouble(c, "height_mm", cfg.height_mm);
        putDouble(c, "mass_kg",   cfg.mass_kg);
        configsEl->InsertEndChild(c);
    }
    dest->InsertEndChild(configsEl);

    // Подписи
    tinyxml2::XMLElement* sigEl = doc->NewElement("Signatures");
    for (const auto& s : signatures) {
        tinyxml2::XMLElement* se = doc->NewElement("Signature");
        putString  (se, "role", s.role);
        putString  (se, "name", s.name);
        putDateTime(se, "date", s.date);
        sigEl->InsertEndChild(se);
    }
    dest->InsertEndChild(sigEl);

    // Привязанные документы — каскадная сериализация DocLink
    tinyxml2::XMLElement* linkedEl = doc->NewElement("LinkedDocuments");
    for (auto& link : linked_documents) {
        tinyxml2::XMLElement* l = doc->NewElement("DocLink");
        link.to_xml(l);
        linkedEl->InsertEndChild(l);
    }
    dest->InsertEndChild(linkedEl);
}

void Part::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    project_id           = getUInt64   (source, "project_id");
    assembly_id          = getUInt64   (source, "assembly_id");
    designation          = getString   (source, "designation");
    name                 = getString   (source, "name");
    description          = getString   (source, "description");
    litera               = getString   (source, "litera");
    organization         = getString   (source, "organization");
    material_instance_id = getOptUInt64(source, "material_instance_id");

    configs.clear();
    if (auto* configsEl = source->FirstChildElement("Configs")) {
        for (auto* c = configsEl->FirstChildElement("Config");
             c; c = c->NextSiblingElement("Config")) {
            PartConfig cfg;
            cfg.config_id = getString(c, "config_id");
            cfg.length_mm = getDouble(c, "length_mm");
            cfg.width_mm  = getDouble(c, "width_mm");
            cfg.height_mm = getDouble(c, "height_mm");
            cfg.mass_kg   = getDouble(c, "mass_kg");
            configs.push_back(std::move(cfg));
        }
    }

    signatures.clear();
    if (auto* sigEl = source->FirstChildElement("Signatures")) {
        for (auto* s = sigEl->FirstChildElement("Signature");
             s; s = s->NextSiblingElement("Signature")) {
            PartSignature ps;
            ps.role = getString  (s, "role");
            ps.name = getString  (s, "name");
            ps.date = getDateTime(s, "date");
            signatures.push_back(std::move(ps));
        }
    }

    linked_documents.clear();
    if (auto* linkedEl = source->FirstChildElement("LinkedDocuments")) {
        for (auto* l = linkedEl->FirstChildElement("DocLink");
             l; l = l->NextSiblingElement("DocLink")) {
            documents::DocLink link;
            link.from_xml(l);
            linked_documents.push_back(std::move(link));
        }
    }
}


} // namespace uniter::contract::design
