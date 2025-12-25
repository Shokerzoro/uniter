#ifndef PURCHASE_H
#define PURCHASE_H

#include "../materialinstance/materialinstancebase.h"
#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <QString>
#include <cstdint>

namespace uniter::resources::purchase {

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
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        const QString& name,
        const QString& description,
        PurchStatus status,
        const MaterialInstanceBase* material
        ) : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),  // ✅ 6 параметров!
        name(name),
        description(description),
        status(status),
        material(material)
    {}

    QString name;
    QString description;
    PurchStatus status;
    const MaterialInstanceBase* material;

    // Сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // purchase


#endif // PURCHASE_H
