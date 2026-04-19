
#include "doc.h"
#include <tinyxml2.h>

namespace uniter::contract::documents {


void Doc::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    // Каскадная сериализация: сначала базовые поля ResourceAbstract.
    ResourceAbstract::to_xml(dest);

    putInt   (dest, "type",        static_cast<int>(type));
    putString(dest, "name",        name);
    putString(dest, "object_key",  object_key);
    putString(dest, "sha256",      sha256);
    putUInt64(dest, "size_bytes",  size_bytes);
    putString(dest, "mime_type",   mime_type);
    putString(dest, "description", description);
}

void Doc::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    type        = static_cast<DocumentType>(
                      getInt(source, "type", static_cast<int>(DocumentType::UNKNOWN)));
    name        = getString(source, "name");
    object_key  = getString(source, "object_key");
    sha256      = getString(source, "sha256");
    size_bytes  = getUInt64(source, "size_bytes");
    mime_type   = getString(source, "mime_type");
    description = getString(source, "description");
}


} // namespace uniter::contract::documents
