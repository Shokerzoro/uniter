#ifndef PROJECT_H
#define PROJECT_H

#include "../resourceabstract.h"
#include <QString>
#include <cstdint>
#include <optional>

namespace uniter::contract::design {

/**
 * @brief Ресурс подсистемы DESIGN — проект.
 *
 * Источник истины для **текущего** состояния конструктора. Проект связан с
 * одной корневой сборкой (`root_assembly_id`) и — опционально — с PDM-проектом
 * (`pdm_project_id`), который управляет собственной историей версий (снэпшотов
 * и дельт). Управление «какой снэпшот head» происходит ВНУТРИ `pdm_project`,
 * `design_project` его не дублирует.
 *
 * Соответствует таблице `design_project` (см. docs/db/design.md).
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
        QString  designation_,
        uint64_t root_assembly_id_,
        std::optional<uint64_t> pdm_project_id_ = std::nullopt)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , name              (std::move(name_))
        , description       (std::move(description_))
        , designation       (std::move(designation_))
        , root_assembly_id  (root_assembly_id_)
        , pdm_project_id    (std::move(pdm_project_id_)) {}

    // Основные поля проекта
    QString name;           // Название проекта
    QString description;    // Описание
    QString designation;    // Обозначение проекта по ЕСКД (верхний уровень)

    // FK на корневую сборку проекта (ровно одна на проект).
    // Хранится как id (не shared_ptr<Assembly>): Assembly загружается отдельно.
    uint64_t root_assembly_id = 0;

    // FK на связанный PDM-проект (таблица `pdm_project`).
    // NULL = у проекта ещё не запущена версионированная PDM-ветка: проект
    // редактируется свободно, снэпшотов нет. После первого запуска PDMManager
    // создаёт PdmProject и проставляет сюда его id.
    //
    // Прежнее поле `active_snapshot_id` удалено: управление head/base snapshot
    // вынесено внутрь `pdm_project` (см. docs/db/pdm.md).
    std::optional<uint64_t> pdm_project_id;

    friend bool operator==(const Project& a, const Project& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.name             == b.name
            && a.description      == b.description
            && a.designation      == b.designation
            && a.root_assembly_id == b.root_assembly_id
            && a.pdm_project_id   == b.pdm_project_id;
    }
    friend bool operator!=(const Project& a, const Project& b) { return !(a == b); }
};

} // namespace uniter::contract::design

#endif // PROJECT_H
