#ifndef TEMPLATESIMPLE_H
#define TEMPLATESIMPLE_H

#include "segment.h"
#include "templatebase.h"
#include <tinyxml2.h>
#include <QString>
#include <map>
#include <vector>
#include <cstdint>

namespace uniter {
namespace contract {
namespace materials {

/**
 * @brief Роль простого стандарта в сборке составного шаблона (Composite).
 *
 * Простые стандарты (TemplateSimple) делятся на четыре категории:
 *
 *   STANDALONE  — самодостаточный стандарт. Его специализация (Instance)
 *                 полностью описывает материал: участвовать в составе
 *                 TemplateComposite он не может ни в верхней, ни в нижней
 *                 части дроби.
 *
 *   ASSORTMENT  — стандарт "сортамента" (геометрический профиль: лист,
 *                 круг, квадрат, швеллер и т.п.). В дроби составного
 *                 шаблона размещается ТОЛЬКО в числителе (top_template_id).
 *                 Содержит обязательный сегмент "профиль сечения".
 *
 *   MATERIAL    — стандарт "на вещество" (марка стали, сплав и т.п.).
 *                 В дроби составного шаблона размещается ТОЛЬКО в
 *                 знаменателе (bottom_template_id).
 *
 *   NONE        — роль не определена / устаревший стандарт /
 *                 неклассифицированный импорт. Используется как
 *                 безопасное значение по умолчанию.
 *
 * Связь "совместимости" между ASSORTMENT и MATERIAL хранится через
 * TemplateSimple::compatible_template_ids: один сортамент указывает
 * множество совместимых материалов и наоборот — связь N:M. В БД на
 * сервере это таблица `template_compatibilities`; в клиентском ресурсе
 * держится денормализованная копия (по аналогии с linked_documents).
 */
enum class StandartType : uint8_t {
    STANDALONE  = 0,
    ASSORTMENT  = 1,
    MATERIAL    = 2,
    NONE        = 255
};

class TemplateSimple : public TemplateBase
{
public:
    TemplateSimple() = default;
    TemplateSimple(
        uint64_t id_,  // Уникальный id шаблона в компании
        QString name_,
        QString description_,
        DimensionType dimension_type_,
        GostSource source_,
        StandartType standart_type_,  // Роль стандарта в дроби Composite (STANDALONE/ASSORTMENT/MATERIAL/NONE)
        GostStandardType standard_type_,  // Тип стандарта (ГОСТ, ОСТ, ГОСТ Р, ТУ и т.п.)
        QString standard_number_,  // Номер стандарта (например, "1050-88", "23-45")
        std::map<uint8_t, SegmentDefinition> prefix_segments_ = {},  // Префиксы (части обозначения, идущие перед номером стандарта)
        std::vector<uint8_t> prefix_order_ = {},  // Порядок следования префиксов
        std::map<uint8_t, SegmentDefinition> suffix_segments_ = {},  // Суффиксы (части обозначения, идущие после номера стандарта)
        std::vector<uint8_t> suffix_order_ = {},  // Порядок следования суффиксов
        std::vector<uint64_t> compatible_template_ids_ = {},  // id совместимых простых шаблонов противоположной роли
        bool is_active_ = true,  // Активен ли шаблон (для архивирования)
        const QDateTime& created_at_ = QDateTime(),
        const QDateTime& updated_at_ = QDateTime(),
        uint32_t version_ = 1,  // Версия шаблона
        uint64_t created_by_ = 0,
        uint64_t updated_by_ = 0)
        : TemplateBase(id_, std::move(name_), std::move(description_), dimension_type_, source_,
                       is_active_, created_at_, updated_at_, version_, created_by_, updated_by_)
        , standart_type(standart_type_)
        , standard_type(standard_type_)  // Тип стандарта и номер (имеют смысл только для простого шаблона)
        , standard_number(std::move(standard_number_))
        , prefix_segments(std::move(prefix_segments_))
        , prefix_order(std::move(prefix_order_))
        , suffix_segments(std::move(suffix_segments_))
        , suffix_order(std::move(suffix_order_))
        , compatible_template_ids(std::move(compatible_template_ids_))
    {}


    GostStandardType standard_type = GostStandardType::GOST; // Тип стандарта
    QString standard_number; // Номер стандарта
    QString year;            // Год редакции стандарта (например "89")

    // Префиксы стандарта
    std::map<uint8_t, SegmentDefinition> prefix_segments;
    std::vector<uint8_t> prefix_order; // Порядок следования префиксов

    // Суффиксы стандарта
    std::map<uint8_t, SegmentDefinition> suffix_segments;
    std::vector<uint8_t> suffix_order; // Порядок следования суффиксов

    // Тип стандарта и связь между сортаментами и материалами
    StandartType standart_type = StandartType::NONE;
    std::vector<uint64_t> compatible_template_ids;

    bool isComposite() const override { return false; }

    // Каскадная сериализация
    virtual void from_xml(tinyxml2::XMLElement* source) override;
    virtual void to_xml(tinyxml2::XMLElement* dest) override;

    friend bool operator==(const TemplateSimple& a, const TemplateSimple& b) {
        return static_cast<const TemplateBase&>(a) == static_cast<const TemplateBase&>(b)
            && a.standart_type            == b.standart_type
            && a.standard_type            == b.standard_type
            && a.standard_number          == b.standard_number
            && a.year                     == b.year
            && a.prefix_segments          == b.prefix_segments
            && a.prefix_order             == b.prefix_order
            && a.suffix_segments          == b.suffix_segments
            && a.suffix_order             == b.suffix_order
            && a.compatible_template_ids  == b.compatible_template_ids;
    }
    friend bool operator!=(const TemplateSimple& a, const TemplateSimple& b) { return !(a == b); }
};

} // materials
} // contract
} // uniter


#endif // TEMPLATESIMPLE_H
