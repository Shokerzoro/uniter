#ifndef PRODUCTIONSUPPLY_H
#define PRODUCTIONSUPPLY_H

#include "../resourceabstract.h"
#include <cstdint>

namespace uniter::contract::production {

/**
 * @brief Поступление по закупочной заявке на конкретную площадку
 *        (ResourceType::PRODUCTION_SUPPLY = 72,
 *         Subsystem::GENERATIVE + GenSubsystem::PRODUCTION).
 *
 * Маппится на таблицу `production_supply` (см. docs/db/production.md).
 *
 * Связывает комплексную заявку supply::PurchaseComplex с площадкой
 * (Plant). Один ProductionSupply = «на площадку plant_id пришла поставка
 * по заявке purchase_complex_id».
 *
 * TODO(на подумать): поля received_at / quantity_received / документы
 * приёмки — в структурной схеме PRODUCTION.pdf их нет, добавятся при
 * расширении. Пока ресурс чисто «связующий».
 */
class ProductionSupply : public ResourceAbstract
{
public:
    ProductionSupply() = default;
    ProductionSupply(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t plant_id_,
        uint64_t purchase_complex_id_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          plant_id(plant_id_),
          purchase_complex_id(purchase_complex_id_)
    {}

    uint64_t plant_id            = 0;   // FK → manager_plant.id
    uint64_t purchase_complex_id = 0;   // FK → supply_purchase_complex.id

    friend bool operator==(const ProductionSupply& a, const ProductionSupply& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.plant_id            == b.plant_id
            && a.purchase_complex_id == b.purchase_complex_id;
    }
    friend bool operator!=(const ProductionSupply& a, const ProductionSupply& b) { return !(a == b); }
};

} // namespace uniter::contract::production

#endif // PRODUCTIONSUPPLY_H
