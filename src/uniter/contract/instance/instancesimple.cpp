#include "instancesimple.h"

namespace uniter {
namespace contract {

void InstanceSimple::setPrefixValue(uint8_t segment_id, const std::string& value) {
    prefix_values[segment_id] = value;
}

void InstanceSimple::setSuffixValue(uint8_t segment_id, const std::string& value) {
    suffix_values[segment_id] = value;
}

std::optional<std::string>
InstanceSimple::getPrefixValue(uint8_t segment_id) const {
    auto it = prefix_values.find(segment_id);
    if (it == prefix_values.end()) return std::nullopt;
    return it->second;
}

std::optional<std::string>
InstanceSimple::getSuffixValue(uint8_t segment_id) const {
    auto it = suffix_values.find(segment_id);
    if (it == suffix_values.end()) return std::nullopt;
    return it->second;
}


} // contract
} // uniter
