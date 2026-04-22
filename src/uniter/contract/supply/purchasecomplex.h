#ifndef PURCHASECOMPLEX_H
#define PURCHASECOMPLEX_H

#include "../resourceabstract.h"
#include "purchase.h"
#include <cstdint>
#include <QString>

namespace uniter::contract::supply {

/**
 * @brief Комплексная закупочная заявка
 *        (ResourceType::PURCHASE_GROUP = 40, Subsystem::PURCHASES).
 *
 * Маппится на таблицу `supply_purchase_complex` (см. docs/db/supply.md).
 *
 * Связь с простыми Purchase — обратная, через FK `purchase_complex_id`
 * в `supply_purchase_simple`. Денормализованного списка id здесь нет:
 * источник истины — FK в Purchase. Список членов заявки материализуется
 * в рантайме запросом SELECT FROM supply_purchase_simple WHERE purchase_complex_id = ?.
 *
 * Это согласовано с SUPPLY.pdf: таблица purchase_complex не содержит FK
 * наружу, все связи идут «снизу вверх» от purchase_simple.
 */
class PurchaseComplex : public ResourceAbstract
{
public:
    PurchaseComplex() = default;
    PurchaseComplex(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString name_,
        QString description_,
        PurchStatus status_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          name(std::move(name_)),
          description(std::move(description_)),
          status(status_) {}

    QString     name;
    QString     description;
    PurchStatus status = PurchStatus::DRAFT;

    friend bool operator==(const PurchaseComplex& a, const PurchaseComplex& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.name        == b.name
            && a.description == b.description
            && a.status      == b.status;
    }
    friend bool operator!=(const PurchaseComplex& a, const PurchaseComplex& b) { return !(a == b); }
};

} // namespace uniter::contract::supply

#endif // PURCHASECOMPLEX_H
