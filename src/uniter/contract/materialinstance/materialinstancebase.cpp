
#include "materialinstancebase.h"
#include <tinyxml2.h>

namespace uniter::contract {

// ---------------- MaterialInstanceBase: каскадная сериализация ----------------
//
// Сначала пишутся поля ResourceAbstract (id/actual/created_at/...), затем
// общие поля экземпляра ссылки: template_id, name, description,
// dimension_type и Quantity (разложена на три optional-поля).
// Наследники (Simple, Composite) вызывают этот метод ПЕРВОЙ строкой своих
// реализаций, чтобы сериализация была каскадной.

void MaterialInstanceBase::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putUInt64(dest, "template_id",    template_id);
    putString(dest, "name",           name);
    putString(dest, "description",    description);
    putInt   (dest, "dimension_type", static_cast<int>(dimension_type));

    // Quantity — три взаимоисключающих поля (в зависимости от dimension_type)
    if (quantity.items.has_value())
        putInt(dest, "quantity_items", *quantity.items);
    if (quantity.length.has_value())
        putDouble(dest, "quantity_length", *quantity.length);
    if (quantity.fig.has_value())
        putDouble(dest, "quantity_area", quantity.fig->area);
}

void MaterialInstanceBase::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    template_id    = getUInt64(source, "template_id");
    name           = getString(source, "name");
    description    = getString(source, "description");
    dimension_type = static_cast<materials::DimensionType>(
                        getInt(source, "dimension_type",
                               static_cast<int>(materials::DimensionType::PIECE)));

    quantity = Quantity{};
    if (source->Attribute("quantity_items"))
        quantity.items = getInt(source, "quantity_items");
    if (source->Attribute("quantity_length"))
        quantity.length = getDouble(source, "quantity_length");
    if (source->Attribute("quantity_area")) {
        figure f;
        f.area = getDouble(source, "quantity_area");
        quantity.fig = f;
    }
}

} // namespace uniter::contract
