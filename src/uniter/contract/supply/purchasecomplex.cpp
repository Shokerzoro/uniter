#include "purchasecomplex.h"
#include <tinyxml2.h>

namespace uniter::contract::supply {

// ---------------- PurchaseComplex: каскадная сериализация ----------------
//
// Комплексная заявка хранит список id простых Purchase. В БД это таблица
// purchase_groups(id, name, description, status) + перекрёстная таблица
// purchase_group_items(group_id, purchase_id). В XML — вложенный список
// <Purchases><Ref id=".../>.

void PurchaseComplex::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putString(dest, "name",        name);
    putString(dest, "description", description);
    putInt   (dest, "status",      static_cast<int>(status));

    auto* doc = dest->GetDocument();
    if (!doc) return;
    auto* listEl = doc->NewElement("Purchases");
    for (auto pid : purchases) {
        auto* ref = doc->NewElement("Ref");
        putUInt64(ref, "id", pid);
        listEl->InsertEndChild(ref);
    }
    dest->InsertEndChild(listEl);
}

void PurchaseComplex::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    name        = getString(source, "name");
    description = getString(source, "description");
    status      = static_cast<PurchStatus>(
                    getInt(source, "status",
                           static_cast<int>(PurchStatus::DRAFT)));

    purchases.clear();
    if (auto* listEl = source->FirstChildElement("Purchases")) {
        for (auto* ref = listEl->FirstChildElement("Ref");
             ref; ref = ref->NextSiblingElement("Ref")) {
            purchases.push_back(getUInt64(ref, "id"));
        }
    }
}


} // namespace uniter::contract::supply
