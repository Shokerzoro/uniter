#ifndef MATERIALTEMPLATECOMPOSITE_H
#define MATERIALTEMPLATECOMPOSITE_H

#include "materialtemplatebase.h"

namespace uniter {
namespace contract {
namespace materials {

class MaterialTemplateComposite : public MaterialTemplateBase {
public:
    MaterialTemplateComposite(
        uint64_t id_,  // Уникальный id шаблона в компании
        QString name_,
        QString description_,
        DimensionType dimension_type_,
        bool is_standalone_,
        GostSource source_,
        uint64_t top_template_id_,  // Id простого шаблона для верхней части
        uint64_t bottom_template_id_,  // Id простого шаблона для нижней части
        bool is_active_ = true,  // Активен ли шаблон (для архивирования)
        const QDateTime& created_at_ = QDateTime(),  // QDateTime created_at;
        const QDateTime& updated_at_ = QDateTime(),  // QDateTime updated_at;
        uint32_t version_ = 1,  // Версия шаблона
        uint64_t created_by_ = 0,
        uint64_t updated_by_ = 0
        )
        : MaterialTemplateBase(id_, std::move(name_), std::move(description_), dimension_type_, is_standalone_, source_,
                               is_active_, created_at_, updated_at_, version_, created_by_, updated_by_)
        , top_template_id(top_template_id_)  // Ссылки на два простых шаблона, которые составляют этот составной материал
        , bottom_template_id(bottom_template_id_)  // Например: тип проката (top_template_id) + материал (bottom_template_id)
    {}
    QString PrefName; // Запись перед стандартом (лист, круг...)
    // Ссылки на два простых шаблона, которые составляют этот составной материал
    // Например: тип проката (top_template_id) + материал (bottom_template_id)
    uint64_t top_template_id; // Id простого шаблона для верхней части
    uint64_t bottom_template_id; // Id простого шаблона для нижней части

    bool isComposite() const override { return true; }

    // Сериализация десериализация
    virtual void from_xml(tinyxml2::XMLElement* source) override;
    virtual void to_xml(tinyxml2::XMLElement* dest) override;
};

} // materials
} // contract
} // uniter


#endif // MATERIALTEMPLATECOMPOSITE_H
