#ifndef INSTANCESIMPLE_H
#define INSTANCESIMPLE_H

#include "instancebase.h"
#include <map>
#include <string>
#include <optional>

namespace uniter {
namespace contract {

/**
 * @brief Экземпляр ссылки на простой материал.
 *
 * Специализирует TemplateSimple конкретными значениями префиксов
 * и суффиксов. Ключ в map — id сегмента из шаблона, значение — выбранное
 * значение этого сегмента (строка).
 */
class InstanceSimple : public InstanceBase {
public:
    InstanceSimple()
        : InstanceBase(ResourceType::MATERIAL_INSTANCE_SIMPLE) {}

    std::map<uint8_t, std::string> prefix_values; // Значения префиксов
    std::map<uint8_t, std::string> suffix_values; // Значения суффиксов

    bool isComposite() const override { return false; }

    void setPrefixValue(uint8_t segment_id, const std::string& value);
    void setSuffixValue(uint8_t segment_id, const std::string& value);
    std::optional<std::string> getPrefixValue(uint8_t segment_id) const;
    std::optional<std::string> getSuffixValue(uint8_t segment_id) const;

    friend bool operator==(const InstanceSimple& a, const InstanceSimple& b) {
        return static_cast<const InstanceBase&>(a) == static_cast<const InstanceBase&>(b)
            && a.prefix_values == b.prefix_values
            && a.suffix_values == b.suffix_values;
    }
    friend bool operator!=(const InstanceSimple& a, const InstanceSimple& b) { return !(a == b); }
};

} // contract
} // uniter

#endif // INSTANCESIMPLE_H
