#ifndef TEMPLATEBASE_H
#define TEMPLATEBASE_H

#include "../resourceabstract.h"
#include "../documents/doclink.h"
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
 * Единая точка истины для всей системы: используется и в Template*,
 * и в Instance* (через `materials::DimensionType`).
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
 * Наследники: TemplateSimple, TemplateComposite.
 *
 * Семантика "standalone / assortment / material" для простых шаблонов
 * вынесена в TemplateSimple::standart_type (см. StandartType в templatesimple.h);
 * поэтому TemplateBase не содержит is_standalone — это свойство применимо
 * только к простым стандартам, но не к составным.
 *
 * Документы стандарта (PDF ГОСТа, выдержка из ТУ и т.п.) — «папка»
 * документов (documents::DocLink). У каждого шаблона ровно одна
 * папка; она свёрнута в поле `doc_link` целиком, без хранения
 * `doc_link_id` в рантайме (в БД, разумеется, FK `doc_link_id`
 * присутствует — см. docs/db/material_instance.md).
 */
class TemplateBase : public ResourceAbstract {
public:
    TemplateBase() = default;
    TemplateBase(
        uint64_t id_,
        QString name_,
        QString description_,
        DimensionType dimension_type_,
        GostSource source_,
        documents::DocLink doc_link_ = {},
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
        , source(source_)
        , version(version_)
        , doc_link(std::move(doc_link_))
    {}

    virtual ~TemplateBase() = default;

    QString       name;             // Краткое человекочитаемое имя
    QString       description;      // Развёрнутое описание, область применения
    DimensionType dimension_type = DimensionType::PIECE; // Размерность материала

    // Метаданные шаблона
    GostSource source  = GostSource::BUILT_IN; // Источник: встроенный или компании-специфичный
    uint32_t   version = 1;                    // Версия шаблона (не SQL-ревизия, а семантическая)

    // «Папка» документов стандарта (PDF ГОСТа и т.п.), свёрнутая целиком.
    // В БД хранится как FK `doc_link_id` на стороне template_simple /
    // template_composite; в рантайме FK не повторяется — содержимое
    // DocLink материализуется DataManager-ом при загрузке шаблона.
    documents::DocLink doc_link;

    // Различение типов
    virtual bool isComposite() const = 0;

    friend bool operator==(const TemplateBase& a, const TemplateBase& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.name           == b.name
            && a.description    == b.description
            && a.dimension_type == b.dimension_type
            && a.source         == b.source
            && a.version        == b.version
            && a.doc_link       == b.doc_link;
    }
    friend bool operator!=(const TemplateBase& a, const TemplateBase& b) { return !(a == b); }
};

} // materials
} // contract
} // uniter

#endif // TEMPLATEBASE_H
