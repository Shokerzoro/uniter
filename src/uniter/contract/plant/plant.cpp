
#include "plant.h"
#include <tinyxml2.h>

namespace uniter::contract::plant {

// Каскадная сериализация Plant (название, описание, тип, локация).

void Plant::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putString(dest, "name",        name);
    putString(dest, "description", description);
    putInt   (dest, "type",        static_cast<int>(type));
    putString(dest, "location",    location);
}

void Plant::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    name        = getString(source, "name");
    description = getString(source, "description");
    type        = static_cast<PlantType>(
                    getInt(source, "type", static_cast<int>(PlantType::PLANT)));
    location    = getString(source, "location");
}


} // namespace uniter::contract::plant
