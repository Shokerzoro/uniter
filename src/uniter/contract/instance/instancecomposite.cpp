#include "instancecomposite.h"
#include <tinyxml2.h>

namespace uniter {
namespace contract {

namespace {

// Локальные хелперы для сериализации map<uint8_t, std::string>.
// Структура XML такая же, как в Simple — см. instancesimple.cpp.

void writeMap(tinyxml2::XMLElement* parent, const char* container,
              const std::map<uint8_t, std::string>& m) {
    auto* doc = parent->GetDocument();
    if (!doc) return;
    auto* el = doc->NewElement(container);
    for (const auto& [seg_id, value] : m) {
        auto* v = doc->NewElement("Value");
        ResourceAbstract::putUInt32(v, "segment_id", static_cast<uint32_t>(seg_id));
        ResourceAbstract::putString(v, "value",      QString::fromStdString(value));
        el->InsertEndChild(v);
    }
    parent->InsertEndChild(el);
}

void readMap(const tinyxml2::XMLElement* parent, const char* container,
             std::map<uint8_t, std::string>& m) {
    m.clear();
    auto* el = parent->FirstChildElement(container);
    if (!el) return;
    for (auto* v = el->FirstChildElement("Value");
         v; v = v->NextSiblingElement("Value")) {
        const uint32_t seg_id = ResourceAbstract::getUInt32(v, "segment_id");
        const QString  value  = ResourceAbstract::getString(v, "value");
        m[static_cast<uint8_t>(seg_id)] = value.toStdString();
    }
}

} // anonymous

void InstanceComposite::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    InstanceBase::to_xml(dest);
    writeMap(dest, "TopValues",    top_values);
    writeMap(dest, "BottomValues", bottom_values);
}

void InstanceComposite::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    InstanceBase::from_xml(source);
    readMap(source, "TopValues",    top_values);
    readMap(source, "BottomValues", bottom_values);
}


} // contract
} // uniter
