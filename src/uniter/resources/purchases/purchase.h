#ifndef PURCHASE_H
#define PURCHASE_H

#include "../materialinstance/materialinstancebase.h"
#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <QString>
#include <cstdint>

namespace uniter {
namespace resources {
namespace purchases {

enum class PurchStatus : uint8_t {
    DRAFT = 0,                  // Черновик (редактируется)
    PLACED = 1,                 // Заказано (отправлено поставщику)
    CANCELLED = 2               // Отменено
};

class Purchase : public ResourceAbstract {
public:
    // Default конструктор
    Purchase() = default;

    // Полный конструктор - точно соответствует базовому классу
    Purchase(
        uint64_t s_id,              // ResourceAbstract::id
        bool actual,                // ResourceAbstract::is_actual
        const QDateTime& c_created_at,  // ResourceAbstract::created_at
        const QDateTime& s_updated_at,  // ResourceAbstract::updated_at
        uint64_t s_created_by,      // ResourceAbstract::created_by
        uint64_t s_updated_by,      // ResourceAbstract::updated_by
        const QString& name,
        const QString& description,
        PurchStatus status,
        uint64_t group,
        uint64_t production_id,
        const MaterialInstanceBase* material
        ) : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),  // ✅ 6 параметров!
        name(name),
        description(description),
        status(status),
        group(group),
        production_id(production_id),
        material(material)
    {}

    uint64_t group;
    uint64_t production_id;
    QString name;
    QString description;
    PurchStatus status;
    const MaterialInstanceBase* material;

    // Сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // purchases
} // resources
} // uniter


#endif // PURCHASE_H
