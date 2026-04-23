#ifndef PURCHASE_H
#define PURCHASE_H

#include "../uniterprotocol.h"
#include "../resourceabstract.h"
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
 * @brief Закупочная заявка (простая)
 *        (ResourceType::PURCHASE = 41, Subsystem::PURCHASES).
 *
 * Маппится на таблицу `supply_purchase_simple` (см. docs/db/supply.md).
 *
 * Связь с материалом:
 *   instance_simple_id XOR instance_composite_id
 * Ровно один из двух FK заполнен — это следствие того, что
 * InstanceSimple и InstanceComposite лежат в БД в разных таблицах
 * (material_instances_simple / material_instances_composite).
 * Проверку инварианта делает DataManager при CREATE/UPDATE.
 *
 * Остальные FK:
 *   - `plant_id`             — площадка-получатель (Subsystem::MANAGER)
 *   - `doc_link_id`          — ссылка на документ (счёт/накладная) через DocLink
 *   - `purchase_complex_id`  — принадлежность к комплексной заявке (optional)
 *
 * ВАЖНО: обратный список `purchases[]` у PurchaseComplex удалён. Члены
 * комплексной заявки находятся SELECT'ом по `purchase_complex_id`;
 * источник истины — FK именно здесь.
 */
class Purchase : public ResourceAbstract {
public:
    Purchase()
        : ResourceAbstract(Subsystem::PURCHASES, GenSubsystemType::NOTGEN, ResourceType::PURCHASE) {}

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
        std::optional<uint64_t> instance_simple_id_    = std::nullopt,
        std::optional<uint64_t> instance_composite_id_ = std::nullopt,
        std::optional<uint64_t> plant_id_              = std::nullopt,
        std::optional<uint64_t> doc_link_id_           = std::nullopt,
        std::optional<uint64_t> purchase_complex_id_   = std::nullopt)
        : ResourceAbstract(Subsystem::PURCHASES, GenSubsystemType::NOTGEN, ResourceType::PURCHASE,
                           s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          name(std::move(name_)),
          description(std::move(description_)),
          status(status_),
          instance_simple_id(instance_simple_id_),
          instance_composite_id(instance_composite_id_),
          plant_id(plant_id_),
          doc_link_id(doc_link_id_),
          purchase_complex_id(purchase_complex_id_)
    {}

    QString                  name;
    QString                  description;
    PurchStatus              status = PurchStatus::DRAFT;

    // --- FK на материал (exactly-one-of-two) --------------------------------
    std::optional<uint64_t>  instance_simple_id;    // FK → material_instances_simple.id
    std::optional<uint64_t>  instance_composite_id; // FK → material_instances_composite.id

    // --- прочие FK -----------------------------------------------------------
    std::optional<uint64_t>  plant_id;             // FK → manager_plant.id (площадка-получатель)
    std::optional<uint64_t>  doc_link_id;          // FK → documents_doc_link.id
    std::optional<uint64_t>  purchase_complex_id;  // FK → supply_purchase_complex.id

    friend bool operator==(const Purchase& a, const Purchase& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.name                  == b.name
            && a.description           == b.description
            && a.status                == b.status
            && a.instance_simple_id    == b.instance_simple_id
            && a.instance_composite_id == b.instance_composite_id
            && a.plant_id              == b.plant_id
            && a.doc_link_id           == b.doc_link_id
            && a.purchase_complex_id   == b.purchase_complex_id;
    }
    friend bool operator!=(const Purchase& a, const Purchase& b) { return !(a == b); }
};

} // namespace uniter::contract::supply

#endif // PURCHASE_H
