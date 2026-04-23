#ifndef ASSEMBLYCONFIG_H
#define ASSEMBLYCONFIG_H

#include "../resourceabstract.h"
#include "designtypes.h"
#include <QString>
#include <cstdint>
#include <vector>

namespace uniter::contract::design {

/**
 * @brief Ресурс подсистемы DESIGN — исполнение сборки.
 *
 * Соответствует таблице `design_assembly_config` (см. docs/db/design.md).
 *
 * Один Assembly имеет N AssemblyConfig. Каждое исполнение хранит:
 *   - FK на родительскую Assembly (поле `assembly_id`);
 *   - код исполнения (`config_code`, «01», «02», ...);
 *   - состав изделия (M:N связи через join-таблицы):
 *       * parts              → `design_assembly_config_parts`
 *       * child_assemblies   → `design_assembly_config_children`
 *       * standard_products  → `design_assembly_config_standard_products`
 *                              (simple instance: один ГОСТ = один крепёж)
 *       * materials          → `design_assembly_config_materials`
 *                              (simple instance: лист/пруток/провод)
 *
 * В C++ M:N-связи материализуются в четыре std::vector<...Ref> при
 * загрузке из БД. CRUD этих векторов — через UniterMessage с
 * `ResourceType::ASSEMBLY_CONFIG` (передаётся конфиг вместе со всеми
 * ссылками целиком; отдельных ресурсов для строк join-таблиц нет).
 *
 * ResourceType::ASSEMBLY_CONFIG = 33.
 *
 * TODO(на подумать): если понадобится тонкий CRUD по отдельной строке
 * join-таблицы (например, увеличить quantity конкретной детали), вынести
 * каждый Ref в самостоятельный ресурс со своим id / ResourceType.
 */
class AssemblyConfig : public ResourceAbstract {
public:
    AssemblyConfig()
        : ResourceAbstract(
              Subsystem::DESIGN,
              GenSubsystemType::NOTGEN,
              ResourceType::ASSEMBLY_CONFIG) {}
    AssemblyConfig(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t assembly_id_,
        QString  config_code_)
        : ResourceAbstract(
              Subsystem::DESIGN,
              GenSubsystemType::NOTGEN,
              ResourceType::ASSEMBLY_CONFIG,
              s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , assembly_id (assembly_id_)
        , config_code (std::move(config_code_)) {}

    // FK на сборку, к которой относится это исполнение.
    uint64_t assembly_id = 0;          // FK → design_assembly.id

    // Код исполнения ("01", "02", ...). В пределах одной assembly — уникален.
    QString config_code;

    // Содержимое исполнения (материализация join-таблиц).
    std::vector<AssemblyConfigPartRef>     parts;              // design_assembly_config_parts
    std::vector<AssemblyConfigChildRef>    child_assemblies;   // design_assembly_config_children
    std::vector<AssemblyConfigStandardRef> standard_products;  // design_assembly_config_standard_products
    std::vector<AssemblyConfigMaterialRef> materials;          // design_assembly_config_materials

    friend bool operator==(const AssemblyConfig& a, const AssemblyConfig& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.assembly_id       == b.assembly_id
            && a.config_code       == b.config_code
            && a.parts             == b.parts
            && a.child_assemblies  == b.child_assemblies
            && a.standard_products == b.standard_products
            && a.materials         == b.materials;
    }
    friend bool operator!=(const AssemblyConfig& a, const AssemblyConfig& b) { return !(a == b); }
};

} // namespace uniter::contract::design

#endif // ASSEMBLYCONFIG_H
