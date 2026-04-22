#ifndef INSTANCEBASE_H
#define INSTANCEBASE_H

#include "../resourceabstract.h"
#include "../material/templatebase.h"
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>

namespace uniter {
namespace contract {

/**
 * @brief Фигура для размерности AREA (плоский материал).
 *
 * Пока хранит только площадь, но структура оставлена на будущее
 * расширение (форма сечения, толщина и т.п.).
 */
struct figure
{
    double area = 0;   // Площадь, м²

    friend bool operator==(const figure& a, const figure& b) {
        return a.area == b.area;
    }
    friend bool operator!=(const figure& a, const figure& b) { return !(a == b); }
};

/**
 * @brief Количество материала в экземпляре ссылки.
 *
 * Три варианта количества соответствуют трём значениям
 * `materials::DimensionType`:
 *   PIECE  → items
 *   LINEAR → length (м)
 *   AREA   → fig.area (м²)
 */
struct Quantity {
    std::optional<int>     items;
    std::optional<double>  length;
    std::optional<figure>  fig;

    friend bool operator==(const Quantity& a, const Quantity& b) {
        return a.items  == b.items
            && a.length == b.length
            && a.fig    == b.fig;
    }
    friend bool operator!=(const Quantity& a, const Quantity& b) { return !(a == b); }
};

/**
 * @brief Базовый класс экземпляра ссылки на материал.
 *
 * По структуре Instance делится на два варианта (simple/composite),
 * которые лежат в разных таблицах БД и имеют разные ResourceType:
 *   - `MATERIAL_INSTANCE_SIMPLE    = 61` → instance/instance_simple
 *   - `MATERIAL_INSTANCE_COMPOSITE = 62` → instance/instance_composite
 * Прежний обобщённый `ResourceType::MATERIAL_INSTANCE = 60` удалён.
 * CRUD идёт через UniterMessage по конкретному ResourceType.
 *
 * Соответствие БД — см. docs/db/material_instance.md.
 *
 * Поле `template_id` — FK на material/template_simple.id (для
 * InstanceSimple) или material/template_composite.id (для
 * InstanceComposite). В отличие от «сворачиваемых» FK (doc_link_id,
 * segment.template_id, ...) этот FK остаётся в рантайм-классе:
 * без него Instance не знает, какой шаблон специализирует.
 *
 * DimensionType берётся из `materials::DimensionType`
 * (`PIECE`/`LINEAR`/`AREA`) — см. material/templatebase.h.
 */
class InstanceBase : public ResourceAbstract {
public:
    InstanceBase() = default;
    InstanceBase(
        uint64_t id_,
        bool actual_,
        const QDateTime& created_at_,
        const QDateTime& updated_at_,
        uint64_t created_by_,
        uint64_t updated_by_,
        uint64_t template_id_,
        QString name_,
        QString description_,
        materials::DimensionType dimension_type_,
        Quantity quantity_)
        : ResourceAbstract(id_, actual_, created_at_, updated_at_, created_by_, updated_by_),
          template_id(template_id_),
          name(std::move(name_)),
          description(std::move(description_)),
          dimension_type(dimension_type_),
          quantity(std::move(quantity_))
    {}
    virtual ~InstanceBase() = default;

    // Связь с шаблоном материала (FK → templates.id)
    uint64_t template_id = 0;

    // Человекочитаемые поля
    QString name;
    QString description;

    // Размерность (копия значения из шаблона для быстрого доступа).
    materials::DimensionType dimension_type = materials::DimensionType::PIECE;

    // Количество согласно dimension_type
    Quantity quantity;

    // Различение типов (Simple vs Composite)
    virtual bool isComposite() const = 0;

    friend bool operator==(const InstanceBase& a, const InstanceBase& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.template_id    == b.template_id
            && a.name           == b.name
            && a.description    == b.description
            && a.dimension_type == b.dimension_type
            && a.quantity       == b.quantity;
    }
    friend bool operator!=(const InstanceBase& a, const InstanceBase& b) { return !(a == b); }
};

} // contract
} // uniter

#endif // INSTANCEBASE_H
