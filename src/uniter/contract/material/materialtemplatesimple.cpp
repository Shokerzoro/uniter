
#include "materialtemplatesimple.h"
#include <tinyxml2.h>

namespace uniter {
namespace contract {
namespace materials {

namespace {

// Сериализация map<uint8_t, SegmentDefinition> как <container><Segment .../></container>.
// Такая плоская структура подходит для реляционной таблицы
// material_template_segments(template_id, segment_id, role, ...).

void writeSegments(tinyxml2::XMLElement* parent, const char* container,
                   const std::map<uint8_t, SegmentDefinition>& segs) {
    auto* doc = parent->GetDocument();
    if (!doc) return;
    auto* el = doc->NewElement(container);
    for (const auto& [key, seg] : segs) {
        auto* s = doc->NewElement("Segment");
        ResourceAbstract::putUInt32(s, "id",          static_cast<uint32_t>(seg.id));
        ResourceAbstract::putUInt32(s, "map_key",     static_cast<uint32_t>(key));
        ResourceAbstract::putString(s, "code",        seg.code);
        ResourceAbstract::putString(s, "name",        seg.name);
        ResourceAbstract::putString(s, "description", seg.description);
        ResourceAbstract::putInt   (s, "value_type",  static_cast<int>(seg.value_type));
        ResourceAbstract::putBool  (s, "is_active",   seg.is_active);

        // allowed_values как вложенный список
        auto* av = doc->NewElement("AllowedValues");
        for (const auto& [av_id, av_val] : seg.allowed_values) {
            auto* v = doc->NewElement("Value");
            ResourceAbstract::putUInt32(v, "id",    static_cast<uint32_t>(av_id));
            ResourceAbstract::putString(v, "value", QString::fromStdString(av_val));
            av->InsertEndChild(v);
        }
        s->InsertEndChild(av);
        el->InsertEndChild(s);
    }
    parent->InsertEndChild(el);
}

void readSegments(const tinyxml2::XMLElement* parent, const char* container,
                  std::map<uint8_t, SegmentDefinition>& segs) {
    segs.clear();
    auto* el = parent->FirstChildElement(container);
    if (!el) return;
    for (auto* s = el->FirstChildElement("Segment");
         s; s = s->NextSiblingElement("Segment")) {
        SegmentDefinition seg;
        seg.id          = static_cast<uint8_t>(ResourceAbstract::getUInt32(s, "id"));
        const uint32_t mapKey = ResourceAbstract::getUInt32(s, "map_key", seg.id);
        seg.code        = ResourceAbstract::getString(s, "code");
        seg.name        = ResourceAbstract::getString(s, "name");
        seg.description = ResourceAbstract::getString(s, "description");
        seg.value_type  = static_cast<SegmentValueType>(
                            ResourceAbstract::getInt(s, "value_type",
                                                     static_cast<int>(SegmentValueType::STRING)));
        seg.is_active   = ResourceAbstract::getBool(s, "is_active", true);

        if (auto* av = s->FirstChildElement("AllowedValues")) {
            for (auto* v = av->FirstChildElement("Value");
                 v; v = v->NextSiblingElement("Value")) {
                const uint32_t av_id = ResourceAbstract::getUInt32(v, "id");
                const QString  val   = ResourceAbstract::getString(v, "value");
                seg.allowed_values[static_cast<uint8_t>(av_id)] = val.toStdString();
            }
        }
        segs[static_cast<uint8_t>(mapKey)] = std::move(seg);
    }
}

void writeOrder(tinyxml2::XMLElement* parent, const char* container,
                const std::vector<uint8_t>& order) {
    auto* doc = parent->GetDocument();
    if (!doc) return;
    auto* el = doc->NewElement(container);
    for (size_t i = 0; i < order.size(); ++i) {
        auto* item = doc->NewElement("Item");
        ResourceAbstract::putUInt32(item, "position", static_cast<uint32_t>(i));
        ResourceAbstract::putUInt32(item, "segment",  static_cast<uint32_t>(order[i]));
        el->InsertEndChild(item);
    }
    parent->InsertEndChild(el);
}

void readOrder(const tinyxml2::XMLElement* parent, const char* container,
               std::vector<uint8_t>& order) {
    order.clear();
    auto* el = parent->FirstChildElement(container);
    if (!el) return;
    for (auto* item = el->FirstChildElement("Item");
         item; item = item->NextSiblingElement("Item")) {
        const uint32_t seg = ResourceAbstract::getUInt32(item, "segment");
        order.push_back(static_cast<uint8_t>(seg));
    }
}

} // anonymous

// ---------------- Каскадная сериализация MaterialTemplateSimple ----------------

void MaterialTemplateSimple::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    MaterialTemplateBase::to_xml(dest);

    putInt   (dest, "standard_type",   static_cast<int>(standard_type));
    putString(dest, "standard_number", standard_number);
    putString(dest, "year",            year);

    writeSegments(dest, "PrefixSegments", prefix_segments);
    writeOrder   (dest, "PrefixOrder",    prefix_order);
    writeSegments(dest, "SuffixSegments", suffix_segments);
    writeOrder   (dest, "SuffixOrder",    suffix_order);
}

void MaterialTemplateSimple::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    MaterialTemplateBase::from_xml(source);

    standard_type   = static_cast<GostStandardType>(
                        getInt(source, "standard_type",
                               static_cast<int>(GostStandardType::GOST)));
    standard_number = getString(source, "standard_number");
    year            = getString(source, "year");

    readSegments(source, "PrefixSegments", prefix_segments);
    readOrder   (source, "PrefixOrder",    prefix_order);
    readSegments(source, "SuffixSegments", suffix_segments);
    readOrder   (source, "SuffixOrder",    suffix_order);
}

} // materials
} // contract
} // uniter
