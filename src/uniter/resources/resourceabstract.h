#ifndef RESOURCEABSTRACT_H
#define RESOURCEABSTRACT_H

#include <tinyxml2.h>
#include <cstdint>
#include <QDateTime>

namespace uniter {
namespace resources {


class ResourceAbstract
{
public:
    ResourceAbstract();
    ResourceAbstract(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by)
        : id(s_id)
        , is_actual(actual)
        , created_at(c_created_at)
        , updated_at(s_updated_at)
        , created_by(s_created_by)
        , updated_by(s_updated_by)
    {}
    ~ResourceAbstract() = default;

    uint64_t id = 0;
    bool is_actual = true;

    QDateTime created_at;
    QDateTime updated_at;
    uint64_t created_by = 0;
    uint64_t updated_by = 0;

    // Сериализация десериализация
    virtual void from_xml(tinyxml2::XMLElement* source);
    virtual void to_xml(tinyxml2::XMLElement* dest);
};



} // resources
} // uniter


#endif // RESOURCEABSTRACT_H
