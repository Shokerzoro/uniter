#ifndef DOCTYPES_H
#define DOCTYPES_H

#include <cstdint>

namespace uniter::contract::documents {

/**
 * @brief Тип конструкторского или технологического документа.
 *
 * Перенесён из (упразднённого) design/designtypes.h. Документ Doc —
 * это инкапсуляция файла в MinIO и его метаданных. Сам Doc хранит
 * `DocumentType` как одно из своих полей; конкретная роль документа
 * (сборочный чертёж, спецификация, 3D-модель и т.п.) задаётся на
 * этапе создания Doc и проверяется валидатором при попытке связать
 * его с ресурсом через DocLink.
 *
 * Значения перечисления расширяемы: при добавлении новой категории
 * документов (например, технологических карт или PDF ГОСТов)
 * достаточно добавить новое значение перед UNKNOWN.
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
 * @brief Тип ресурса, к которому привязан Doc через DocLink.
 *
 * Дублирует часть `ResourceType` из uniterprotocol.h, но намеренно:
 * DocLink хранит только те target-типы, которые имеют смысл для
 * прикрепления документов. Расширяется только по явной необходимости.
 *
 * Соответствует колонке `target_type` таблицы `doc_links`.
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
