#ifndef MATERIALINSTANCEBASE_H
#define MATERIALINSTANCEBASE_H

#include <tinyxml2.h>
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>


namespace uniter {
namespace contract {

enum class DimensionType : uint8_t {
    PIECE,
    LINEAR,
    PLAIN
};

struct figure
{
    double area = 0;
};

struct Quantity {
    std::optional<int> items;
    std::optional<double> length;
    std::optional<figure> fig;
};

class MaterialInstanceBase {
public:
    MaterialInstanceBase(
        uint64_t template_id_,
        const QString& name_,
        const QString& description_,
        DimensionType dimension_type_,
        const Quantity& quantity_
        ) : template_id(template_id_),
        name(name_),
        description(description_),
        dimension_type(dimension_type_),
        quantity(quantity_)
    {}
    virtual ~MaterialInstanceBase() = default; // Идентификация

    uint64_t template_id; // Ссылка на MaterialTemplateBase (ГОСТ)
    QString name; // Человекочитаемое имя материала
    QString description; // Описание
    DimensionType dimension_type; // Копируется из template для быстрого доступа
    Quantity quantity;

    // Виртуальные методы
    virtual bool isComposite() const = 0;

    // Сериализация десериализация
    virtual void from_xml(tinyxml2::XMLElement* source);
    virtual void to_xml(tinyxml2::XMLElement* dest);
};



} // contract
} // uniter


#endif // MATERIALINSTANCEBASE_H
