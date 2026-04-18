#ifndef MATERIALINSTANCECOMPOSITE_H
#define MATERIALINSTANCECOMPOSITE_H

#include "materialinstancebase.h"
#include <tinyxml2.h>
#include <map>
#include <string>

namespace uniter {
namespace contract {


/**
 * @brief Экземпляр ссылки на составной материал.
 *
 * Специализирует MaterialTemplateComposite (двухчастный шаблон через дробь)
 * отдельно для верхней и нижней частей.
 */
class MaterialInstanceComposite : public MaterialInstanceBase {
public:
    MaterialInstanceComposite() = default;

    // Заполненные значения сегментов для составного материала
    std::map<uint8_t, std::string> top_values;    // Значения верхней части
    std::map<uint8_t, std::string> bottom_values; // Значения нижней части

    bool isComposite() const override { return true; }

    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // contract
} // uniter


#endif // MATERIALINSTANCECOMPOSITE_H
