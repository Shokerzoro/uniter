

#include "materialinstancesimple.h"
#include <tinyxml2.h>

namespace uniter {
namespace contract {

void MaterialInstanceSimple::setPrefixValue(uint8_t segment_id, const std::string& value) {
    prefix_values[segment_id] = value;
}

void MaterialInstanceSimple::setSuffixValue(uint8_t segment_id, const std::string& value) {
    suffix_values[segment_id] = value;
}

std::optional<std::string>
MaterialInstanceSimple::getPrefixValue(uint8_t segment_id) const {
    auto it = prefix_values.find(segment_id);
    if (it == prefix_values.end()) return std::nullopt;
    return it->second;
}

std::optional<std::string>
MaterialInstanceSimple::getSuffixValue(uint8_t segment_id) const {
    auto it = suffix_values.find(segment_id);
    if (it == suffix_values.end()) return std::nullopt;
    return it->second;
}

// ---------------- Каскадная сериализация ----------------
// Сначала записываются поля базового класса (MaterialInstanceBase + его
// ResourceAbstract), затем вложенные элементы <PrefixValues> и
// <SuffixValues> с парами <Value segment_id="" value=""/>. Такая структура
// напрямую ложится в реляционные таблицы:
//   material_instance_prefix_values(instance_id, segment_id, value)
//   material_instance_suffix_values(instance_id, segment_id, value)

namespace {

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

void MaterialInstanceSimple::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    MaterialInstanceBase::to_xml(dest);
    writeMap(dest, "PrefixValues", prefix_values);
    writeMap(dest, "SuffixValues", suffix_values);
}

void MaterialInstanceSimple::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    MaterialInstanceBase::from_xml(source);
    readMap(source, "PrefixValues", prefix_values);
    readMap(source, "SuffixValues", suffix_values);
}


} // contract
} // uniter
