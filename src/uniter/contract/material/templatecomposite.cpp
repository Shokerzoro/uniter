
#include "templatecomposite.h"
#include <tinyxml2.h>

namespace uniter {
namespace contract {
namespace materials {

// ---------------- Каскадная сериализация TemplateComposite ----------------

void TemplateComposite::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    TemplateBase::to_xml(dest);

    putString(dest, "pref_name",          PrefName);
    putUInt64(dest, "top_template_id",    top_template_id);
    putUInt64(dest, "bottom_template_id", bottom_template_id);
}

void TemplateComposite::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    TemplateBase::from_xml(source);

    PrefName           = getString(source, "pref_name");
    top_template_id    = getUInt64(source, "top_template_id");
    bottom_template_id = getUInt64(source, "bottom_template_id");
}

} // materials
} // contract
} // uniter
