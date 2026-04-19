#ifndef TEMPLATECOMPOSITE_H
#define TEMPLATECOMPOSITE_H

#include "templatebase.h"

namespace uniter::contract::materials {

/**
 * @brief Составной шаблон материала — два простых стандарта через дробь.
 *
 * Инвариант по ролям (StandartType, см. templatesimple.h):
 *   - top_template_id    — простой шаблон с standart_type == ASSORTMENT
 *                          (профиль сечения: лист, круг, квадрат, …);
 *   - bottom_template_id — простой шаблон с standart_type == MATERIAL
 *                          (марка стали / сплав / вещество);
 *   - STANDALONE-стандарты не могут входить в Composite ни в каком виде.
 *
 * Связь "совместимости" (какой ASSORTMENT с каким MATERIAL допустим)
 * хранится в TemplateSimple::compatible_template_ids — Composite её
 * не дублирует, но сервер обязан проверять её при создании/обновлении.
 */
class TemplateComposite : public TemplateBase {
public:
    TemplateComposite() = default;
    TemplateComposite(
        uint64_t id_,  // Уникальный id шаблона в компании
        QString name_,
        QString description_,
        DimensionType dimension_type_,
        GostSource source_,
        uint64_t top_template_id_,  // Id простого шаблона для верхней части (ASSORTMENT)
        uint64_t bottom_template_id_,  // Id простого шаблона для нижней части (MATERIAL)
        bool is_active_ = true,  // Активен ли шаблон (для архивирования)
        const QDateTime& created_at_ = QDateTime(),
        const QDateTime& updated_at_ = QDateTime(),
        uint32_t version_ = 1,  // Версия шаблона
        uint64_t created_by_ = 0,
        uint64_t updated_by_ = 0
        )
        : TemplateBase(id_, std::move(name_), std::move(description_), dimension_type_, source_,
                       is_active_, created_at_, updated_at_, version_, created_by_, updated_by_)
        , top_template_id(top_template_id_)
        , bottom_template_id(bottom_template_id_)
    {}

    QString PrefName; // Запись перед дробью (лист, круг...) — берётся из ASSORTMENT-стандарта
    uint64_t top_template_id    = 0; // FK → templates.id (простой, ASSORTMENT)
    uint64_t bottom_template_id = 0; // FK → templates.id (простой, MATERIAL)

    bool isComposite() const override { return true; }

    // Каскадная сериализация
    virtual void from_xml(tinyxml2::XMLElement* source) override;
    virtual void to_xml(tinyxml2::XMLElement* dest) override;
};

} // namespace uniter::contract::materials

#endif // TEMPLATECOMPOSITE_H
