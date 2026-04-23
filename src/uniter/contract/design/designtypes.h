#ifndef DESIGNTYPES_H
#define DESIGNTYPES_H

#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>

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

// ----------------------------------------------------------------------------
// Ссылочные структуры для AssemblyConfig (материализация join-таблиц).
// Сами структуры живут не в отдельных таблицах, а в join-таблицах
// (design_assembly_config_parts / _children / _standard_products / _materials),
// поэтому не являются ресурсами (ResourceAbstract).
// ----------------------------------------------------------------------------

/**
 * @brief Ссылка на деталь в исполнении сборки.
 * Соответствует строке join-таблицы `design_assembly_config_parts`.
 */
struct AssemblyConfigPartRef {
    uint64_t part_id          = 0;    // FK → design_part.id
    uint32_t quantity         = 1;    // Количество вхождений
    QString  part_config_code;        // Выбранное исполнение детали ("01", "02", ... или пусто)

    friend bool operator==(const AssemblyConfigPartRef& a, const AssemblyConfigPartRef& b) {
        return a.part_id          == b.part_id
            && a.quantity         == b.quantity
            && a.part_config_code == b.part_config_code;
    }
    friend bool operator!=(const AssemblyConfigPartRef& a, const AssemblyConfigPartRef& b) { return !(a == b); }
};

/**
 * @brief Ссылка на дочернюю сборку в исполнении родительской сборки.
 * Соответствует строке join-таблицы `design_assembly_config_children`.
 */
struct AssemblyConfigChildRef {
    uint64_t child_assembly_id = 0;   // FK → design_assembly.id
    uint32_t quantity          = 1;
    QString  child_config_code;       // Выбранное исполнение дочерней сборки

    friend bool operator==(const AssemblyConfigChildRef& a, const AssemblyConfigChildRef& b) {
        return a.child_assembly_id == b.child_assembly_id
            && a.quantity          == b.quantity
            && a.child_config_code == b.child_config_code;
    }
    friend bool operator!=(const AssemblyConfigChildRef& a, const AssemblyConfigChildRef& b) { return !(a == b); }
};

/**
 * @brief Ссылка на стандартное изделие (болт, шайба, заклёпка и т.п.).
 * Соответствует строке join-таблицы `design_assembly_config_standard_products`.
 *
 * По ЕСКД стандартное изделие полностью описывается ОДНИМ стандартом
 * (Например ГОСТ 7798-70 на болт с шестигранной головкой): то есть
 * это **simple** Instance. «Составной» Composite Template/Instance
 * должен резервироваться строго для полуфабрикатов вида «сортамент +
 * марка вещества» (напр. «лист / сталь 09З2С»).
 */
struct AssemblyConfigStandardRef {
    uint64_t instance_simple_id = 0;     // FK → material_instances_simple.id
    uint32_t quantity           = 1;

    friend bool operator==(const AssemblyConfigStandardRef& a, const AssemblyConfigStandardRef& b) {
        return a.instance_simple_id == b.instance_simple_id
            && a.quantity           == b.quantity;
    }
    friend bool operator!=(const AssemblyConfigStandardRef& a, const AssemblyConfigStandardRef& b) { return !(a == b); }
};

/**
 * @brief Ссылка на материал в исполнении сборки (лист, пруток, провод и т.п.).
 * Соответствует строке join-таблицы `design_assembly_config_materials`.
 *
 * По ЕСКД «материал» в спецификации — это конкретная позиция,
 * описанная одним стандартом (напр. «Провод МГ 1,5 ГОСТ 6323-79»), это
 * **simple** Instance. Полуфабрикаты вида «сортамент + марка стали»
 * (composite) — отдельный кейс, но тоже редкий и здесь не учитывается.
 *
 * Количество зависит от DimensionType шаблона: items (штуки),
 * length (м), area (м²).
 */
struct AssemblyConfigMaterialRef {
    uint64_t instance_simple_id = 0;     // FK → material_instances_simple.id
    std::optional<uint32_t> quantity_items;   // для PIECE
    std::optional<double>   quantity_length;  // для LINEAR
    std::optional<double>   quantity_area;    // для AREA

    friend bool operator==(const AssemblyConfigMaterialRef& a, const AssemblyConfigMaterialRef& b) {
        return a.instance_simple_id == b.instance_simple_id
            && a.quantity_items     == b.quantity_items
            && a.quantity_length    == b.quantity_length
            && a.quantity_area      == b.quantity_area;
    }
    friend bool operator!=(const AssemblyConfigMaterialRef& a, const AssemblyConfigMaterialRef& b) { return !(a == b); }
};

} // namespace uniter::contract::design

#endif // DESIGNTYPES_H
