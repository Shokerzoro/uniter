
#include "productionstock.h"
#include <tinyxml2.h>

namespace uniter::contract::production {


void ProductionStock::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putUInt64(dest, "material_instance_id", material_instance_id);
    putUInt64(dest, "plant_id",             plant_id);
    putInt   (dest, "quantity_items",       quantity_items);
    putDouble(dest, "quantity_length",      quantity_length);
    putDouble(dest, "quantity_area",        quantity_area);
}

void ProductionStock::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    material_instance_id = getUInt64(source, "material_instance_id");
    plant_id             = getUInt64(source, "plant_id");
    quantity_items       = getInt   (source, "quantity_items");
    quantity_length      = getDouble(source, "quantity_length");
    quantity_area        = getDouble(source, "quantity_area");
}


} // namespace uniter::contract::production
