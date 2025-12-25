#ifndef PLANT_H
#define PLANT_H

#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <QString>
#include <cstdint>

namespace uniter::resources::plant {

// Тип производства
enum class PlantType : uint8_t {
    PLANT = 0,      // Завод
    WAREHOUSE = 1,  // Склад
    VIRTUAL = 2     // Виртуальное (группа)
};

class Plant : public ResourceAbstract
{
public:
    Plant(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString name_,
        QString description_,
        PlantType type_,
        QString location_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
        name(std::move(name_)),
        description(std::move(description_)),
        type(type_),
        location(std::move(location_)) {}

    QString name;
    QString description;
    PlantType type = PlantType::PLANT;
    QString location;

    // Сериализация десериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};



} // plant

#endif // PLANT_H
