#ifndef PROJECT_H
#define PROJECT_H

#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <QString>
#include <cstdint>
#include <optional>

namespace uniter::contract::design {

/**
 * @brief Ресурс подсистемы DESIGN — проект.
 *
 * Источник истины для текущего состояния конструктора. Проект связан с одной
 * корневой сборкой (`root_assembly_id`) и с «официальной» версией через
 * `active_snapshot_id` (FK → Snapshot со статусом APPROVED).
 *
 * Соответствует таблице `projects` (см. docs/pdm_design_architecture.md §1.2).
 */
class Project : public ResourceAbstract {
public:
    Project() = default;
    Project(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString  name_,
        QString  description_,
        QString  projectcode_,
        QString  rootdirectory_,
        uint64_t root_assembly_id_,
        std::optional<uint64_t> active_snapshot_id_ = std::nullopt)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , name           (std::move(name_))
        , description    (std::move(description_))
        , projectcode    (std::move(projectcode_))
        , rootdirectory  (std::move(rootdirectory_))
        , root_assembly_id(root_assembly_id_)
        , active_snapshot_id(std::move(active_snapshot_id_)) {}

    // Основные поля проекта
    QString name;             // Название проекта
    QString description;      // Описание
    QString projectcode;      // Код проекта (ЕСКД-обозначение верхнего уровня)
    QString rootdirectory;    // Корневая папка КД (локальный путь или URI)

    // FK на корневую сборку проекта (ровно одна на проект).
    // Хранится как id, а не shared_ptr<Assembly>: Assembly загружается отдельно.
    uint64_t root_assembly_id = 0;

    // FK на «официальную» версию проекта (Snapshot со статусом APPROVED).
    // NULL пока ни один Snapshot не утверждён.
    std::optional<uint64_t> active_snapshot_id;

    // Каскадная сериализация (базовые поля пишутся через ResourceAbstract::to_xml)
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml  (tinyxml2::XMLElement* dest)   override;
};


} // namespace uniter::contract::design

#endif // PROJECT_H
