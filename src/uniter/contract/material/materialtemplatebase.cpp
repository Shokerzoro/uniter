
#include "materialtemplatebase.h"
#include <tinyxml2.h>

namespace uniter::contract::materials {

// ---------------- MaterialTemplateBase: каскадная сериализация ----------------

void MaterialTemplateBase::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putString(dest, "name",           name);
    putString(dest, "description",    description);
    putInt   (dest, "dimension_type", static_cast<int>(dimension_type));
    putBool  (dest, "is_standalone",  is_standalone);
    putInt   (dest, "source",         static_cast<int>(source));
    putUInt32(dest, "version",        version);
}

void MaterialTemplateBase::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    name           = getString(source, "name");
    description    = getString(source, "description");
    dimension_type = static_cast<DimensionType>(
                        getInt(source, "dimension_type",
                               static_cast<int>(DimensionType::PIECE)));
    is_standalone  = getBool (source, "is_standalone", true);
    this->source   = static_cast<GostSource>(
                        getInt(source, "source",
                               static_cast<int>(GostSource::BUILT_IN)));
    version        = getUInt32(source, "version", 1);
}


} // namespace uniter::contract::materials
