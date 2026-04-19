
#include "doclink.h"
#include <tinyxml2.h>

namespace uniter::contract::documents {


void DocLink::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putUInt64(dest, "doc_id",      doc_id);
    putInt   (dest, "target_type", static_cast<int>(target_type));
    putUInt64(dest, "target_id",   target_id);
    putString(dest, "role",        role);
    putUInt32(dest, "position",    position);
}

void DocLink::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    doc_id      = getUInt64(source, "doc_id");
    target_type = static_cast<DocLinkTargetType>(
                      getInt(source, "target_type",
                             static_cast<int>(DocLinkTargetType::UNKNOWN)));
    target_id   = getUInt64(source, "target_id");
    role        = getString(source, "role");
    position    = getUInt32(source, "position");
}


} // namespace uniter::contract::documents
