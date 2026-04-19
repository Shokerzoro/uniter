#ifndef INSTANCESIMPLE_H
#define INSTANCESIMPLE_H

#include "instancebase.h"
#include <tinyxml2.h>
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
    InstanceSimple() = default;

    std::map<uint8_t, std::string> prefix_values; // Значения префиксов
    std::map<uint8_t, std::string> suffix_values; // Значения суффиксов

    bool isComposite() const override { return false; }

    void setPrefixValue(uint8_t segment_id, const std::string& value);
    void setSuffixValue(uint8_t segment_id, const std::string& value);
    std::optional<std::string> getPrefixValue(uint8_t segment_id) const;
    std::optional<std::string> getSuffixValue(uint8_t segment_id) const;

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // contract
} // uniter

#endif // INSTANCESIMPLE_H
