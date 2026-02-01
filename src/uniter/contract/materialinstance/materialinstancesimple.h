#ifndef MATERIALINSTANCESIMPLE_H
#define MATERIALINSTANCESIMPLE_H

#include "materialinstancebase.h"
#include <tinyxml2.h>
#include <map>
#include <optional>

namespace uniter {
namespace contract {

class MaterialInstanceSimple : public MaterialInstanceBase {
public:

    std::map<uint8_t, std::string> prefix_values; // Значения префиксов
    std::map<uint8_t, std::string> suffix_values; // Значения суффиксов
    bool isComposite() const override { return false; }

    void setPrefixValue(uint8_t segment_id, const uint8_t value);
    void setSuffixValue(uint8_t segment_id, const uint8_t value);
    std::optional<std::string> getPrefixValue(uint8_t segment_id) const;
    std::optional<std::string> getSuffixValue(uint8_t segment_id) const;

    // Сериализация десериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // contract
} // uniter

#endif // MATERIALINSTANCESIMPLE_H
