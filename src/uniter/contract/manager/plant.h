#ifndef PLANT_H
#define PLANT_H

#include "../resourceabstract.h"
#include "managertypes.h"
#include <QString>
#include <cstdint>

namespace uniter::contract::manager {

/**
 * @brief Производственная площадка (ResourceType::PRODUCTION = 11, Subsystem::MANAGER).
 *
 * Генеративный ресурс: создание Plant порождает GenSubsystem::PRODUCTION
 * c `GenSubsystemId == plant.id`. Под каждой площадкой появляются свои
 * ProductionTask + ProductionStock (см. contract/production/).
 *
 * TODO(refactor): поле `location` сейчас — свободная строка. Если потребуется
 *   структура (адрес, координаты, почтовый индекс), вынести в отдельный
 *   ресурс PlantAddress с FK plant_id.
 */
class Plant : public ResourceAbstract
{
public:
    Plant()
        : ResourceAbstract(Subsystem::MANAGER, GenSubsystemType::NOTGEN, ResourceType::PRODUCTION) {}

    Plant(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString name_,
        QString description_,
        PlantType type_,
        QString location_)
        : ResourceAbstract(Subsystem::MANAGER, GenSubsystemType::NOTGEN, ResourceType::PRODUCTION,
                           s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
        name(std::move(name_)),
        description(std::move(description_)),
        type(type_),
        location(std::move(location_)) {}

    QString name;
    QString description;
    PlantType type = PlantType::PLANT;
    QString location;

    friend bool operator==(const Plant& a, const Plant& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.name        == b.name
            && a.description == b.description
            && a.type        == b.type
            && a.location    == b.location;
    }
    friend bool operator!=(const Plant& a, const Plant& b) { return !(a == b); }
};

} // namespace uniter::contract::manager

// Backward-compat: старый namespace contract::plant использовался в некоторых
// заголовках. Alias позволяет не ломать внешние include до полного перехода.
namespace uniter::contract {
namespace plant = manager;
}

#endif // PLANT_H
