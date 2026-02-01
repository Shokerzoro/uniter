#ifndef TASK_H
#define TASK_H

#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <QString>
#include <cstdint>

namespace uniter::contract::plant {

// Статус задания
enum class TaskStatus : uint8_t {
    PLANNED = 0,     // Запланировано
    IN_PROGRESS = 1, // В работе
    PAUSED = 2,      // Приостановлено
    COMPLETED = 3,   // Завершено
    CANCELLED = 4    // Отменено
};

struct ProductionPartNode
{
    ProductionPartNode() = default;

    // Идентификация
    uint64_t assembly_id = 0;      // ID сборки из Project
    uint64_t project_id = 0;       // ID проекта-владельца
    uint64_t part_id = 0;

    // Общие данные
    TaskStatus status = TaskStatus::PLANNED;
    uint64_t quantity_planned = 0;
    uint64_t quantity_completed = 0;

    // Планируемые даты для аналитики
    QDateTime planned_start;
    QDateTime planned_end;
    std::optional<QDateTime> actual_start;
    std::optional<QDateTime> actual_end;

    QDateTime updated_at;
};

struct ProductionAssemblyNode {

    // Идентификация
    uint64_t assembly_id = 0;      // ID сборки из Project
    uint64_t project_id = 0;       // ID проекта-владельца

    // Общие данные
    TaskStatus status = TaskStatus::PLANNED;
    uint64_t quantity_planned = 0;
    uint64_t quantity_completed = 0;

    // Планируемые даты для аналитики
    QDateTime planned_start;
    QDateTime planned_end;
    std::optional<QDateTime> actual_start;
    std::optional<QDateTime> actual_end;

    // Дочерние узлы (рекурсивно)
    std::map<uint64_t, std::vector<ProductionAssemblyNode>> child_assemblies;
    std::map<uint64_t, std::vector<ProductionPartNode>> parts;

    QDateTime updated_at;
};


class Task : public ResourceAbstract
{
public:
    Task(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString code_,
        QString name_,
        uint64_t project_id_,
        uint64_t quantity_,
        TaskStatus status_,
        uint64_t plant_id_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
        code(std::move(code_)),
        name(std::move(name_)),
        project_id(project_id_),
        quantity(quantity_),
        status(status_),
        plant_id(plant_id_) {}

    Task() = default;

    // Идентификатор и название задания
    QString code;
    QString name;

    // Связь с проектом
    uint64_t project_id;
    uint64_t plant_id;

    // Количество (единиц продукции)
    uint64_t quantity;
    TaskStatus status = TaskStatus::PLANNED;
    std::vector<std::shared_ptr<ProductionAssemblyNode>> root_assembly;

    // Период выполнения
    QDateTime planned_start;
    QDateTime planned_end;
    std::optional<QDateTime> actual_start;
    std::optional<QDateTime> actual_end;

    // Сериализация десериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};



} // plant

#endif // TASK_H
