#ifndef MATERIALTEMPLATESIMPLE_H
#define MATERIALTEMPLATESIMPLE_H

#include "segment.h"
#include "materialtemplatebase.h"
#include <tinyxml2.h>
#include <QString>
#include <map>
#include <vector>

namespace uniter {
namespace resources {
namespace materials {

class MaterialTemplateSimple : public MaterialTemplateBase
{
public:
    MaterialTemplateSimple(
        uint64_t id_,  // Уникальный id шаблона в компании
        QString name_,
        QString description_,
        DimensionType dimension_type_,
        bool is_standalone_,
        GostSource source_,
        GostStandardType standard_type_,  // Тип стандарта (ГОСТ, ОСТ, ГОСТ Р, ТУ и т.п.)
        QString standard_number_,  // Номер стандарта (например, "1050-88", "23-45")
        std::map<uint8_t, SegmentDefinition> prefix_segments_ = {},  // Префиксы (части обозначения, идущие перед номером стандарта)
        std::vector<uint8_t> prefix_order_ = {},  // Порядок следования префиксов
        std::map<uint8_t, SegmentDefinition> suffix_segments_ = {},  // Суффиксы (части обозначения, идущие после номера стандарта)
        std::vector<uint8_t> suffix_order_ = {},  // Порядок следования суффиксов
        bool is_active_ = true,  // Активен ли шаблон (для архивирования)
        const QDateTime& created_at_ = QDateTime(),  // QDateTime created_at;
        const QDateTime& updated_at_ = QDateTime(),  // QDateTime updated_at;
        uint32_t version_ = 1,  // Версия шаблона  <<-- ИСПРАВЛЕНО: добавлена запятая
        uint64_t created_by_ = 0,
        uint64_t updated_by_ = 0)
        : MaterialTemplateBase(id_, std::move(name_), std::move(description_), dimension_type_, is_standalone_, source_,
                               is_active_, created_at_, updated_at_, version_, created_by_, updated_by_)
        , standard_type(standard_type_)  // Тип стандарта и номер (имеют смысл только для простого шаблона)
        , standard_number(std::move(standard_number_))  // Номер стандарта (например, "1050-88", "23-45")
        , prefix_segments(std::move(prefix_segments_))  // Ключ: id сегмента (имеет смысл только внутри этого шаблона)
        , prefix_order(std::move(prefix_order_))  // Порядок следования префиксов
        , suffix_segments(std::move(suffix_segments_))  // Суффиксы (части обозначения, идущие после номера стандарта)
        , suffix_order(std::move(suffix_order_))  // Порядок следования суффиксов
    {}

    // Тип стандарта и номер (имеют смысл только для простого шаблона)
    GostStandardType standard_type; // Тип стандарта (ГОСТ, ОСТ, ГОСТ Р, ТУ и т.п.)
    QString standard_number; // Номер стандарта (например, "1050-88", "23-45")

    // Префиксы (части обозначения, идущие перед номером стандарта)
    // Ключ: id сегмента (имеет смысл только внутри этого шаблона)
    // Значение: определение сегмента
    std::map<uint8_t, SegmentDefinition> prefix_segments;
    std::vector<uint8_t> prefix_order; // Порядок следования префиксов

    // Суффиксы (части обозначения, идущие после номера стандарта)
    std::map<uint8_t, SegmentDefinition> suffix_segments;
    std::vector<uint8_t> suffix_order; // Порядок следования суффиксов

    bool isComposite() const override { return false; }

    // Сериализация десериализация
    virtual void from_xml(tinyxml2::XMLElement* source) override;
    virtual void to_xml(tinyxml2::XMLElement* dest) override;
};

} // materials
} // resources
} // uniter


#endif // MATERIALTEMPLATESIMPLE_H
