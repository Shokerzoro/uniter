
#include "purchase.h"
#include <tinyxml2.h>

namespace uniter::contract::supply {


void Purchase::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putString   (dest, "name",                 name);
    putString   (dest, "description",          description);
    putInt      (dest, "status",               static_cast<int>(status));
    putOptUInt64(dest, "material_instance_id", material_instance_id);
}

void Purchase::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    name                 = getString   (source, "name");
    description          = getString   (source, "description");
    status               = static_cast<PurchStatus>(
                              getInt(source, "status",
                                     static_cast<int>(PurchStatus::DRAFT)));
    material_instance_id = getOptUInt64(source, "material_instance_id");
}

} // namespace uniter::contract::supply
