#ifndef RESOURCEABSTRACT_H
#define RESOURCEABSTRACT_H


#include <cstdint>
#include <QDateTime>

namespace uniter {
namespace resources {


class ResourceAbstract
{
public:
    ResourceAbstract(uint64_t s_id) : id(s_id);
    virtual ~ResourceAbstract() = 0;
    uint64_t id = 0;
    bool is_actual = true;

    QDateTime created_at;
    QDateTime updated_at;
    uint64_t created_by = 0;
    uint64_t updated_by = 0;

    virtual void from_xml() = 0;
    virtual void to_xml() = 0;
};


} // resources
} // uniter


#endif // RESOURCEABSTRACT_H
