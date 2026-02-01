#ifndef PURCHASECOMPLEX_H
#define PURCHASECOMPLEX_H

#include "../resourceabstract.h"
#include "purchase.h"
#include <tinyxml2.h>
#include <vector>
#include <cstdint>

namespace uniter::contract::supply {


class PurchaseComplex : public ResourceAbstract
{
public:
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
        std::vector<uint64_t> purchases_) :
        ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
        name(name_),
        description(description_),
        status(status_),
        purchases(purchases_) {}


    QString name;
    QString description;
    PurchStatus status;
    std::vector<uint64_t> purchases;

    // Сериализация десериализация
    virtual void from_xml(tinyxml2::XMLElement* source) override;
    virtual void to_xml(tinyxml2::XMLElement* dest) override;
};


} // supply


#endif // PURCHASECOMPLEX_H
