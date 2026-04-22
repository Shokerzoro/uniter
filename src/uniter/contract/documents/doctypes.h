#ifndef DOCTYPES_H
#define DOCTYPES_H

#include <cstdint>

namespace uniter::contract::documents {

/**
 * @brief Тип конструкторского или технологического документа.
 *
 * Хранится в каждом Doc и задаётся в момент его создания. Расширяется только
 * по явной необходимости.
 */
enum class DocumentType : uint8_t {
    ASSEMBLY_DRAWING    = 0,   // Сборочный чертёж
    MOUNTING_DRAWING    = 1,   // Монтажный чертёж
    MODEL_3D            = 2,   // 3D-модель
    PART_DRAWING        = 3,   // Чертёж детали
    SPECIFICATION       = 4,   // Спецификация
    STANDARD_PDF        = 5,   // PDF стандарта (ГОСТ / ОСТ / ТУ)
    UNKNOWN             = 255
};

/**
 * @brief Тип целевого ресурса DocLink.
 *
 * DocLink — «папка» документов, прикреплённая к одному целевому ресурсу.
 * `DocLinkTargetType` + `target_id` у владельца задают то, к чему прикреплена
 * эта папка. Сам DocLink в БД (см. DOCUMENTS.pdf) хранит только
 * `id` и `DocLinkTargetType`; фактическая обратная ссылка — `doc_link_id`
 * на стороне владельца (Assembly.doc_link_id, Part.doc_link_id,
 * MaterialTemplate*.doc_link_id и т.п.). Здесь перечисляются только те типы
 * целей, для которых такая «папка» имеет смысл.
 */
enum class DocLinkTargetType : uint8_t {
    ASSEMBLY                     = 0,   // Assembly
    PART                         = 1,   // Part
    PROJECT                      = 2,   // Project (документ уровня проекта)
    MATERIAL_TEMPLATE_SIMPLE     = 3,   // Простой шаблон материала (PDF ГОСТа)
    MATERIAL_TEMPLATE_COMPOSITE  = 4,   // Составной шаблон материала
    UNKNOWN                      = 255
};


} // namespace uniter::contract::documents

#endif // DOCTYPES_H
