#ifndef PURCHASECOMPLEX_H
#define PURCHASECOMPLEX_H

#include "../resourceabstract.h"
#include "purchase.h"
#include <tinyxml2.h>
#include <vector>
#include <cstdint>
#include <QString>

namespace uniter::contract::supply {


/**
 * @brief Комплексная закупочная заявка.
 *
 * Группирует несколько простых Purchase по id. В БД маппится на таблицу
 * `purchase_groups` + перекрёстная `purchase_group_items`.
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
        PurchStatus status_,
        std::vector<uint64_t> purchases_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          name(std::move(name_)),
          description(std::move(description_)),
          status(status_),
          purchases(std::move(purchases_)) {}


    QString               name;
    QString               description;
    PurchStatus           status = PurchStatus::DRAFT;
    std::vector<uint64_t> purchases;   // FK → purchases.id

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // namespace uniter::contract::supply


#endif // PURCHASECOMPLEX_H
