
#include "project.h"
#include <tinyxml2.h>

namespace uniter::contract::design {


void Project::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    // Базовые поля ResourceAbstract (каскадная сериализация)
    ResourceAbstract::to_xml(dest);

    // Специфичные поля Project
    putString   (dest, "name",               name);
    putString   (dest, "description",        description);
    putString   (dest, "projectdesignation", projectdesignation);
    putUInt64   (dest, "root_assembly_id",   root_assembly_id);
    putOptUInt64(dest, "active_snapshot_id", active_snapshot_id);
}

void Project::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    name               = getString   (source, "name");
    description        = getString   (source, "description");
    projectdesignation = getString   (source, "projectdesignation");
    root_assembly_id   = getUInt64   (source, "root_assembly_id");
    active_snapshot_id = getOptUInt64(source, "active_snapshot_id");
}


} // namespace uniter::contract::design
