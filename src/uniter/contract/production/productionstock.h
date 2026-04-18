#ifndef PRODUCTIONSTOCK_H
#define PRODUCTIONSTOCK_H

#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <cstdint>

namespace uniter::contract::production {

/**
 * @brief Позиция складского запаса (ResourceType::PRODUCTION_STOCK = 71).
 *
 * Связывает конкретный MaterialInstance с конкретной площадкой (Plant)
 * и показывает доступное количество этого материала на этой площадке.
 *
 * Реляционное представление (таблица `production_stock`):
 *   id               PK
 *   material_instance_id  FK → material_instances.id
 *   plant_id              FK → plants.id
 *   quantity_items        INTEGER    (для DimensionType::PIECE)
 *   quantity_length       REAL       (для DimensionType::LINEAR, м)
 *   quantity_area         REAL       (для DimensionType::AREA,   м²)
 *
 * Какое из трёх количественных полей активно — определяется
 * `dimension_type` у соответствующего MaterialInstance/Template.
 * Храним все три для однообразного маппинга на SQLite; незаполненные
 * поля остаются нулевыми.
 */
class ProductionStock : public ResourceAbstract
{
public:
    ProductionStock() = default;
    ProductionStock(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t material_instance_id_,
        uint64_t plant_id_,
        int      quantity_items_  = 0,
        double   quantity_length_ = 0.0,
        double   quantity_area_   = 0.0)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          material_instance_id(material_instance_id_),
          plant_id(plant_id_),
          quantity_items(quantity_items_),
          quantity_length(quantity_length_),
          quantity_area(quantity_area_)
    {}

    uint64_t material_instance_id = 0;  // FK → material_instances.id
    uint64_t plant_id             = 0;  // FK → plants.id

    // Количественные поля (активность определяется dimension_type материала)
    int    quantity_items  = 0;    // штук (PIECE)
    double quantity_length = 0.0;  // м    (LINEAR)
    double quantity_area   = 0.0;  // м²   (AREA)

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // namespace uniter::contract::production

#endif // PRODUCTIONSTOCK_H
