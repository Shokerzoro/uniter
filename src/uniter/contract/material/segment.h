#ifndef SEGMENT_H
#define SEGMENT_H

#include <QString>
#include <cstdint>
#include <map>
#include <string>

namespace uniter {
namespace contract {
namespace materials {

/**
 * @brief Тип значения сегмента обозначения.
 */
enum class SegmentValueType : uint8_t {
    STRING  = 0, // Произвольная строка
    ENUM    = 1, // Выбор из списка
    NUMBER  = 2, // Число (int или double)
    CODE    = 3  // Код (специальный формат)
};

/**
 * @brief Позиция сегмента в обозначении (префикс / суффикс).
 *
 * В БД хранится в столбце `material/segment.type`. В рантайме
 * тип позиции не дублируется в SegmentDefinition — вместо этого
 * сегменты раскладываются в два разных map внутри TemplateSimple:
 * `prefix_segments` (SegmentType::PREFIX) и `suffix_segments`
 * (SegmentType::SUFFIX). См. docs/db/material_instance.md.
 */
enum class SegmentType : uint8_t {
    PREFIX  = 0,
    SUFFIX  = 1
};

/**
 * @brief Определение сегмента обозначения материала.
 *
 * Сворачивается из двух таблиц БД:
 *
 *   material/segment        — сам сегмент (id, template_id FK, type, order,
 *                             code, name, description, value_type, is_active)
 *   material/segment_value  — допустимые значения для сегмента с ENUM
 *                             (id, segment_id FK, code → значение)
 *
 * В рантайме обе таблицы сворачиваются в SegmentDefinition:
 * значения `segment_value.*` складываются в `allowed_values`.
 * FK (segment.template_id, segment_value.segment_id) в рантайм-классе
 * не хранятся — они восстановимы из контекста: SegmentDefinition
 * живёт либо в `TemplateSimple::prefix_segments`, либо в
 * `TemplateSimple::suffix_segments`, а template_id равен id родительского
 * TemplateSimple.
 *
 * Ресурсы в uniterprotocol.h:
 *   ResourceType::SEGMENT        — таблица material/segment
 *   ResourceType::SEGMENT_VALUE  — таблица material/segment_value
 * Оба имеют собственный CRUD, так как состав сегментов и их значений
 * может меняться независимо от самого шаблона.
 */
struct SegmentDefinition {
    uint8_t id;
    QString code;                                        // Машинное имя (для логики)
    QString name;                                        // Человекочитаемое имя
    QString description;                                 // Описание сегмента
    SegmentValueType value_type;                         // Тип значения
    std::map<uint8_t, std::string> allowed_values;       // Свёрнутые segment_value.value по segment_value.id
    bool is_active = true;                               // true = активен, false = deprecated

    friend bool operator==(const SegmentDefinition& a, const SegmentDefinition& b) {
        return a.id             == b.id
            && a.code           == b.code
            && a.name           == b.name
            && a.description    == b.description
            && a.value_type     == b.value_type
            && a.allowed_values == b.allowed_values
            && a.is_active      == b.is_active;
    }
    friend bool operator!=(const SegmentDefinition& a, const SegmentDefinition& b) { return !(a == b); }
};


} // materials
} // contract
} // uniter

#endif // SEGMENT_H
