#ifndef PURCHASE_H
#define PURCHASE_H

#include "../resourceabstract.h"
#include <tinyxml2.h>
#include <QString>
#include <cstdint>
#include <optional>

namespace uniter::contract::supply {

enum class PurchStatus : uint8_t {
    DRAFT     = 0, // Черновик (редактируется)
    PLACED    = 1, // Заказано (отправлено поставщику)
    CANCELLED = 2  // Отменено
};

/**
 * @brief Закупочная заявка (простая).
 *
 * По принципу реляционной БД связь с MaterialInstance хранится через
 * `material_instance_id` (FK), а не raw pointer — это исключает висячие
 * ссылки и упрощает маппинг на таблицу `purchases`.
 */
class Purchase : public ResourceAbstract {
public:
    Purchase() = default;

    Purchase(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        QString name_,
        QString description_,
        PurchStatus status_,
        std::optional<uint64_t> material_instance_id_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          name(std::move(name_)),
          description(std::move(description_)),
          status(status_),
          material_instance_id(material_instance_id_)
    {}

    QString                  name;
    QString                  description;
    PurchStatus              status = PurchStatus::DRAFT;
    std::optional<uint64_t>  material_instance_id;   // FK → material_instances.id

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


} // namespace uniter::contract::supply


#endif // PURCHASE_H
