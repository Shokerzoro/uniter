#ifndef MATERIALTEMPLATEBASE_H
#define MATERIALTEMPLATEBASE_H

#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <QString>
#include <QDateTime>
#include <cstdint>

namespace uniter {
namespace contract {
namespace materials {

enum class GostStandardType : uint8_t {
    GOST    = 0, // ГОСТ
    OST     = 1, // ОСТ
    GOST_R  = 2, // ГОСТ Р
    TU      = 3, // ТУ (техническое условие)
    SNIP    = 4, // СНиП
    OTHER   = 5  // Другое
};

/**
 * @brief Размерность материала.
 *
 * Единая точка истины для всей системы: используется и в MaterialTemplate*,
 * и в MaterialInstance* (через `materials::DimensionType`). Ранее был
 * продублирован в materialinstance/materialinstancebase.h — дубликат убран.
 */
enum class DimensionType : uint8_t {
    PIECE   = 0, // Поштучный
    LINEAR  = 1, // Линейный (м)
    AREA    = 2  // Плоский (м²)
};

enum class GostSource : uint8_t {
    BUILT_IN            = 0, // Предустановленный стандарт
    COMPANY_SPECIFIC    = 1  // Специфичный для компании
};

/**
 * @brief Базовый класс шаблона материала.
 *
 * Наследники: MaterialTemplateSimple, MaterialTemplateComposite.
 * Каскадная сериализация: реализации наследников обязаны первой строкой
 * вызывать MaterialTemplateBase::to_xml / from_xml, которые в свою очередь
 * первой строкой вызывают ResourceAbstract::to_xml / from_xml.
 */
class MaterialTemplateBase : public ResourceAbstract {
public:
    MaterialTemplateBase() = default;
    MaterialTemplateBase(
        uint64_t id_,
        QString name_,
        QString description_,
        DimensionType dimension_type_,
        bool is_standalone_,
        GostSource source_,
        bool is_active_ = true,
        const QDateTime& created_at_ = QDateTime(),
        const QDateTime& updated_at_ = QDateTime(),
        uint32_t version_ = 1,
        uint64_t created_by_ = 0,
        uint64_t updated_by_ = 0)
        : ResourceAbstract(id_, is_active_, created_at_, updated_at_, created_by_, updated_by_)
        , name(std::move(name_))
        , description(std::move(description_))
        , dimension_type(dimension_type_)
        , is_standalone(is_standalone_)
        , source(source_)
        , version(version_)
    {}

    virtual ~MaterialTemplateBase() = default;

    QString       name;             // Краткое человекочитаемое имя
    QString       description;      // Развёрнутое описание, область применения
    DimensionType dimension_type = DimensionType::PIECE; // Размерность материала
    bool          is_standalone  = true;   // Может ли этот ГОСТ самостоятельно описать изделие

    // Метаданные шаблона
    GostSource source  = GostSource::BUILT_IN; // Источник: встроенный или компании-специфичный
    uint32_t   version = 1;                    // Версия шаблона (не SQL-ревизия, а семантическая)

    // Различение типов
    virtual bool isComposite() const = 0;

    // Каскадная сериализация (базовая реализация — пишет общие поля
    // шаблона поверх полей ResourceAbstract). Наследники переопределяют
    // и первой строкой вызывают MaterialTemplateBase::to_xml / from_xml.
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // materials
} // contract
} // uniter

#endif // MATERIALTEMPLATEBASE_H
