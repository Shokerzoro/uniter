#ifndef MATERIALINSTANCECOMPOSITE_H
#define MATERIALINSTANCECOMPOSITE_H

#include "materialinstancebase.h"
#include <tinyxml2.h>

namespace uniter {
namespace contract {


class MaterialInstanceComposite : public MaterialInstanceBase {
public:
    // Заполненные значения сегментов для составного материала
    std::map<uint8_t, std::string> top_values; // Значения верхней части
    std::map<uint8_t, std::string> bottom_values; // Значения нижней части

    bool isComposite() const override { return true; }

    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // contract
} // uniter


#endif // MATERIALINSTANCECOMPOSITE_H
