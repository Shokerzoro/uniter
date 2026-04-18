#ifndef MATERIALINSTANCEBASE_H
#define MATERIALINSTANCEBASE_H

#include "../resourceabstract.h"
#include "../material/materialtemplatebase.h"
#include <tinyxml2.h>
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
};

/**
 * @brief Базовый класс экземпляра ссылки на материал.
 *
 * MATERIAL_INSTANCE = 60 в ResourceType, поэтому должен быть полноценным
 * ресурсом, унаследованным от ResourceAbstract (иначе CRUD через
 * UniterMessage не сработает). DimensionType берётся из
 * `materials::DimensionType` (`PIECE`/`LINEAR`/`AREA`) — см.
 * material/materialtemplatebase.h.
 */
class MaterialInstanceBase : public ResourceAbstract {
public:
    MaterialInstanceBase() = default;
    MaterialInstanceBase(
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
    virtual ~MaterialInstanceBase() = default;

    // Связь с шаблоном материала (FK → material_templates.id)
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

    // Каскадная сериализация — наследники вызывают
    //   MaterialInstanceBase::to_xml(dest);
    //   MaterialInstanceBase::from_xml(source);
    // ПЕРВОЙ строкой своих реализаций.
    virtual void from_xml(tinyxml2::XMLElement* source) override;
    virtual void to_xml(tinyxml2::XMLElement* dest) override;
};



} // contract
} // uniter


#endif // MATERIALINSTANCEBASE_H
