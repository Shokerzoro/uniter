#ifndef PARTCONFIG_H
#define PARTCONFIG_H

#include "../resourceabstract.h"
#include <QString>
#include <cstdint>

namespace uniter::contract::design {

/**
 * @brief Ресурс подсистемы DESIGN — исполнение детали.
 *
 * Соответствует таблице `design_part_config` (см. docs/db/design.md).
 *
 * Раньше был вложенной struct внутри Part — теперь полноценный ресурс
 * (наследник ResourceAbstract) с собственным id и ResourceType::PART_CONFIG.
 * Это нужно для CRUD по одному исполнению (исполнения детали добавляются
 * и правятся независимо друг от друга).
 *
 * Один Part имеет N PartConfig; каждое исполнение — свой габарит и масса.
 * Код исполнения (`config_code`) уникален в пределах одной детали.
 *
 * ResourceType::PART_CONFIG = 34.
 */
class PartConfig : public ResourceAbstract {
public:
    PartConfig()
        : ResourceAbstract(
              Subsystem::DESIGN,
              GenSubsystemType::NOTGEN,
              ResourceType::PART_CONFIG) {}
    PartConfig(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t part_id_,
        QString  config_code_,
        double   length_mm_ = 0.0,
        double   width_mm_  = 0.0,
        double   height_mm_ = 0.0,
        double   mass_kg_   = 0.0)
        : ResourceAbstract(
              Subsystem::DESIGN,
              GenSubsystemType::NOTGEN,
              ResourceType::PART_CONFIG,
              s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , part_id     (part_id_)
        , config_code (std::move(config_code_))
        , length_mm   (length_mm_)
        , width_mm    (width_mm_)
        , height_mm   (height_mm_)
        , mass_kg     (mass_kg_) {}

    // FK на деталь, к которой относится это исполнение.
    uint64_t part_id = 0;      // FK → design_part.id

    // Код исполнения ("01", "02", ...). В пределах одной детали — уникален.
    QString config_code;

    // Геометрия/масса исполнения
    double length_mm = 0.0;
    double width_mm  = 0.0;
    double height_mm = 0.0;
    double mass_kg   = 0.0;

    friend bool operator==(const PartConfig& a, const PartConfig& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.part_id     == b.part_id
            && a.config_code == b.config_code
            && a.length_mm   == b.length_mm
            && a.width_mm    == b.width_mm
            && a.height_mm   == b.height_mm
            && a.mass_kg     == b.mass_kg;
    }
    friend bool operator!=(const PartConfig& a, const PartConfig& b) { return !(a == b); }
};

} // namespace uniter::contract::design

#endif // PARTCONFIG_H
