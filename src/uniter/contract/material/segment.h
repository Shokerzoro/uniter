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
};


} // materials
} // contract
} // uniter

#endif // SEGMENT_H
