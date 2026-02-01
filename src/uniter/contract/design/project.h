#ifndef PROJECT_H
#define PROJECT_H

#include "../resourceabstract.h"
#include "assembly.h"
#include <tinyxml2.h>

namespace uniter::contract::design {


class Project : public ResourceAbstract {
public:
    Project(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString name_,
        QString description_,
        QString projectcode_,
        QString rootdirectory_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
        name(std::move(name_)),
        description(std::move(description_)),
        projectcode(std::move(projectcode_)),
        rootdirectory(std::move(rootdirectory_)) {}

    Project() = default;

    // Основные поля проекта
    QString name;
    QString description;
    QString projectcode;
    QString rootdirectory;

    // Корневая сборка (одна на проект)
    std::shared_ptr<Assembly> root_assembly;

    // Сериализация десериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};



} // design

#endif // PROJECT_H
