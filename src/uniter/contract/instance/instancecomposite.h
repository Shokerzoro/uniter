#ifndef INSTANCECOMPOSITE_H
#define INSTANCECOMPOSITE_H

#include "instancebase.h"
#include <map>
#include <string>

namespace uniter {
namespace contract {

/**
 * @brief Экземпляр ссылки на составной материал.
 *
 * Специализирует TemplateComposite (двухчастный шаблон через дробь)
 * отдельно для верхней и нижней частей.
 */
class InstanceComposite : public InstanceBase {
public:
    InstanceComposite() = default;

    // Заполненные значения сегментов для составного материала
    std::map<uint8_t, std::string> top_values;    // Значения верхней части
    std::map<uint8_t, std::string> bottom_values; // Значения нижней части

    bool isComposite() const override { return true; }

    friend bool operator==(const InstanceComposite& a, const InstanceComposite& b) {
        return static_cast<const InstanceBase&>(a) == static_cast<const InstanceBase&>(b)
            && a.top_values    == b.top_values
            && a.bottom_values == b.bottom_values;
    }
    friend bool operator!=(const InstanceComposite& a, const InstanceComposite& b) { return !(a == b); }
};

} // contract
} // uniter

#endif // INSTANCECOMPOSITE_H
