#ifndef DESIGNTYPES_H
#define DESIGNTYPES_H

#include <QString>
#include <QDateTime>
#include <cstdint>

namespace uniter::contract::design {

/**
 * @brief Тип сборки.
 *
 * REAL    — обнаружена спецификация (сборка подтверждена КД);
 * VIRTUAL — спецификация не найдена, но сборка выведена из контекста ЕСКД.
 */
enum class AssemblyType : uint8_t {
    REAL    = 0,
    VIRTUAL = 1
};

/**
 * @brief Исполнение детали.
 *
 * Соответствует таблице `part_configs` (см. docs/pdm_design_architecture.md §1.4).
 * Одна запись = одно исполнение одной детали.
 */
struct PartConfig {
    QString config_id;      // "01", "02", ...
    double  length_mm = 0;
    double  width_mm  = 0;
    double  height_mm = 0;
    double  mass_kg   = 0;

    friend bool operator==(const PartConfig& a, const PartConfig& b) {
        return a.config_id == b.config_id
            && a.length_mm == b.length_mm
            && a.width_mm  == b.width_mm
            && a.height_mm == b.height_mm
            && a.mass_kg   == b.mass_kg;
    }
    friend bool operator!=(const PartConfig& a, const PartConfig& b) { return !(a == b); }
};

/**
 * @brief Подпись на чертеже.
 *
 * Соответствует таблице `part_signatures` (см. docs/pdm_design_architecture.md §1.4).
 */
struct PartSignature {
    QString   role;   // "Разработал", "Проверил", ...
    QString   name;   // ФИО
    QDateTime date;   // Дата подписи

    friend bool operator==(const PartSignature& a, const PartSignature& b) {
        return a.role == b.role && a.name == b.name && a.date == b.date;
    }
    friend bool operator!=(const PartSignature& a, const PartSignature& b) { return !(a == b); }
};

} // namespace uniter::contract::design

#endif // DESIGNTYPES_H
