#ifndef SEGMENT_H
#define SEGMENT_H

#include <QString>
#include <cstdint>
#include <map>

namespace uniter {
namespace contract {
namespace materials {

enum class SegmentValueType : uint8_t {
    STRING  = 0, // Произвольная строка
    ENUM    = 1, // Выбор из списка
    NUMBER  = 2, // Число (int или double)
    CODE    = 3 // Код (специальный формат)
};

struct SegmentDefinition {
    uint8_t id;
    QString code; // Машинное имя (для логики)
    QString name; // Человекочитаемое имя
    QString description; // Описание сегмента
    SegmentValueType value_type;// Тип значения
    std::map<uint8_t, std::string> allowed_values; // Статус сегмента
    bool is_active = true; // true = активен, false = удалён (deprecated)

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
