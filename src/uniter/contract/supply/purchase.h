#ifndef PURCHASE_H
#define PURCHASE_H

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
 * Связь с MaterialInstance — через `material_instance_id` (FK).
 *
 * Дополнительные FK (согласно parsed.DataSheme-2.txt):
 *   - `plant_id`             — площадка-назначение (на какую Plant приходит материал)
 *   - `doc_link_id`          — ссылка на связанный документ (счёт/накладная) через DocLink
 *   - `purchase_complex_id`  — обратная ссылка на комплексную заявку, если эта Purchase
 *                              входит в состав PurchaseComplex.purchases[]
 *
 * NOTE: все FK optional — простая Purchase может жить без привязки к площадке,
 * документу или комплексной заявке. Комплексная заявка при этом также ссылается
 * на её членов через PurchaseComplex.purchases[] — денормализация намеренная
 * (ускоряет запросы в обе стороны). TODO(refactor): если вырастет требование
 * к целостности, вынести связь в отдельный ресурс PurchaseComplexItem
 * (join-table), и удалить и `purchases[]`, и `purchase_complex_id`.
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
        std::optional<uint64_t> material_instance_id_,
        std::optional<uint64_t> plant_id_            = std::nullopt,
        std::optional<uint64_t> doc_link_id_         = std::nullopt,
        std::optional<uint64_t> purchase_complex_id_ = std::nullopt)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by),
          name(std::move(name_)),
          description(std::move(description_)),
          status(status_),
          material_instance_id(material_instance_id_),
          plant_id(plant_id_),
          doc_link_id(doc_link_id_),
          purchase_complex_id(purchase_complex_id_)
    {}

    QString                  name;
    QString                  description;
    PurchStatus              status = PurchStatus::DRAFT;

    // --- FK на связанные ресурсы -------------------------------------------
    std::optional<uint64_t>  material_instance_id; // FK → material_instances.id
    std::optional<uint64_t>  plant_id;             // FK → plants.id (площадка-получатель)
    std::optional<uint64_t>  doc_link_id;          // FK → doc_links.id (счёт / накладная)
    std::optional<uint64_t>  purchase_complex_id;  // FK → purchase_complexes.id (если входит)

    friend bool operator==(const Purchase& a, const Purchase& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.name                 == b.name
            && a.description          == b.description
            && a.status               == b.status
            && a.material_instance_id == b.material_instance_id
            && a.plant_id             == b.plant_id
            && a.doc_link_id          == b.doc_link_id
            && a.purchase_complex_id  == b.purchase_complex_id;
    }
    friend bool operator!=(const Purchase& a, const Purchase& b) { return !(a == b); }
};

} // namespace uniter::contract::supply

#endif // PURCHASE_H
