#ifndef PRODUCTIONSTOCK_H
#define PRODUCTIONSTOCK_H

#include "../uniterprotocol.h"
#include "../resourceabstract.h"
#include <cstdint>
#include <optional>

namespace uniter::contract::production {

/**
 * @brief Позиция складского запаса
 *        (ResourceType::PRODUCTION_STOCK = 71,
 *         Subsystem::GENERATIVE + GenSubsystem::PRODUCTION).
 *
 * Маппится на таблицу `production_stock` (см. docs/db/production.md).
 *
 * Связь с материалом: instance_simple_id XOR instance_composite_id
 * (exactly-one-of-two, проверяет DataManager).
 *
 * Какое из трёх количественных полей активно — определяется
 * `dimension_type` соответствующего Instance. Храним все три для
 * однообразного маппинга на SQLite; неактивные остаются нулевыми.
 */
class ProductionStock : public ResourceAbstract
{
public:
    ProductionStock()
        : ResourceAbstract(Subsystem::GENERATIVE, GenSubsystemType::PRODUCTION,
                           ResourceType::PRODUCTION_STOCK) {}
    ProductionStock(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t plant_id_,
        std::optional<uint64_t> instance_simple_id_    = std::nullopt,
        std::optional<uint64_t> instance_composite_id_ = std::nullopt,
        int      quantity_items_  = 0,
        double   quantity_length_ = 0.0,
        double   quantity_area_   = 0.0)
        : ResourceAbstract(Subsystem::GENERATIVE, GenSubsystemType::PRODUCTION,
                           ResourceType::PRODUCTION_STOCK,
                           s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          plant_id(plant_id_),
          instance_simple_id(instance_simple_id_),
          instance_composite_id(instance_composite_id_),
          quantity_items(quantity_items_),
          quantity_length(quantity_length_),
          quantity_area(quantity_area_)
    {}

    // Площадка-владелец (генеративная подсистема PRODUCTION)
    uint64_t plant_id = 0;                            // FK → manager_plant.id

    // Материал (exactly-one-of-two)
    std::optional<uint64_t> instance_simple_id;       // FK → material_instances_simple.id
    std::optional<uint64_t> instance_composite_id;    // FK → material_instances_composite.id

    // Количественные поля (активность определяется dimension_type материала)
    int    quantity_items  = 0;    // штук (PIECE)
    double quantity_length = 0.0;  // м    (LINEAR)
    double quantity_area   = 0.0;  // м²   (AREA)

    friend bool operator==(const ProductionStock& a, const ProductionStock& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.plant_id              == b.plant_id
            && a.instance_simple_id    == b.instance_simple_id
            && a.instance_composite_id == b.instance_composite_id
            && a.quantity_items        == b.quantity_items
            && a.quantity_length       == b.quantity_length
            && a.quantity_area         == b.quantity_area;
    }
    friend bool operator!=(const ProductionStock& a, const ProductionStock& b) { return !(a == b); }
};

} // namespace uniter::contract::production

#endif // PRODUCTIONSTOCK_H
