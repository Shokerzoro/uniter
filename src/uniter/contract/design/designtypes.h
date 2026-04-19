#ifndef DESIGNTYPES_H
#define DESIGNTYPES_H

#include <QString>
#include <QDateTime>
#include <cstdint>

// TODO: изменить название файла на doctypes.h

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
 * @brief Тип конструкторского документа.
 *
 * Используется в delta_file_changes для описания того, какой именно файл
 * изменился в конкретном изменении Delta. Перенесено из (упразднённого)
 * fileversion.h — история версий файлов теперь ведётся в Delta.
 */
enum class DocumentType : uint8_t {
    ASSEMBLY_DRAWING    = 0,   // Сборочный чертёж
    MOUNTING_DRAWING    = 1,   // Монтажный чертёж
    MODEL_3D            = 2,   // 3D-модель
    PART_DRAWING        = 3,   // Чертёж детали
    SPECIFICATION       = 4,   // Спецификация
    UNKNOWN             = 5
};

struct DocRef { // TODO: public ResourceAbstract, вынести в отдельную п подсистему
    // тип документа
    // object_key
    // sha256

    // id? Если будет хранится в отдельной таблице? И тогда еще новая подиситема. Мне это нравится. Лучше дробить, чем копить все вместе
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
};

} // namespace uniter::contract::design

#endif // DESIGNTYPES_H
