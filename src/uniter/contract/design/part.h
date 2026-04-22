#ifndef PART_H
#define PART_H

#include "../resourceabstract.h"
#include "../documents/doclink.h"
#include "designtypes.h"
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>
#include <vector>

namespace uniter::contract::design {

/**
 * @brief Ресурс подсистемы DESIGN — деталь (общие данные).
 *
 * Соответствует таблице `design_part` (см. docs/db/design.md).
 *
 * Деталь НЕ принадлежит конкретной сборке: она входит в исполнения сборок
 * через join-таблицу `design_assembly_config_parts` (N:M). Поэтому здесь
 * нет `assembly_id`.
 *
 * Исполнения детали (`PartConfig`) вынесены в отдельный ресурс (класс
 * `PartConfig` наследник ResourceAbstract), таблица `design_part_config`.
 * Здесь в классе полей configs[] больше не хранится — CRUD по исполнениям
 * идёт через UniterMessage с `ResourceType::PART_CONFIG`.
 *
 * Материал детали — ровно ОДНА из двух FK:
 *   - `instance_simple_id`    → material_instances_simple.id  (лист/пруток/…)
 *   - `instance_composite_id` → material_instances_composite.id (лист+покрытие, …)
 * Инвариант XOR поддерживается DataManager (можно «ни одного» для отвлечённых
 * деталей уровня спецификации, но одновременно оба — запрещено).
 */
class Part : public ResourceAbstract {
public:
    Part() = default;
    Part(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t project_id_,
        QString  designation_,
        QString  name_,
        QString  description_)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , project_id  (project_id_)
        , designation (std::move(designation_))
        , name        (std::move(name_))
        , description (std::move(description_)) {}

    // FK на проект (классификация деталей по проектам).
    uint64_t project_id = 0;    // FK → design_project.id

    // Атрибуты ЕСКД
    QString designation;        // Обозначение ЕСКД (например "СБ-001-01")
    QString name;
    QString description;
    QString litera;             // Литера КД (О, О1, А, ...)
    QString organization;       // Организация-разработчик

    // Материал детали: ровно одна из двух FK (или обе NULL для отвлечённых).
    // Инвариант: (instance_simple_id IS NULL) XOR (instance_composite_id IS NULL)
    //            OR  (оба NULL).
    std::optional<uint64_t> instance_simple_id;      // FK → material_instances_simple.id
    std::optional<uint64_t> instance_composite_id;   // FK → material_instances_composite.id

    // Подписи на чертеже (таблица `design_part_signatures`, прикладная).
    // TODO(на подумать): вынести в отдельный ресурс PartSignature (ResourceType)
    // если понадобится CRUD по отдельной подписи. Пока — вектор внутри Part.
    std::vector<PartSignature> signatures;

    // Привязанные документы (денормализация doc_links по target_type=PART).
    std::vector<documents::DocLink> linked_documents;

    // Отсутствует (вынесено в отдельные сущности):
    //   - assembly_id         → N:M через design_assembly_config_parts
    //   - configs[]           → отдельный ресурс PartConfig с FK part_id
    //   - material_instance_id (старое поле) → разделено на simple/composite
    //     согласно ЕСКД-типу материала.

    friend bool operator==(const Part& a, const Part& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.project_id            == b.project_id
            && a.designation           == b.designation
            && a.name                  == b.name
            && a.description           == b.description
            && a.litera                == b.litera
            && a.organization          == b.organization
            && a.instance_simple_id    == b.instance_simple_id
            && a.instance_composite_id == b.instance_composite_id
            && a.signatures            == b.signatures
            && a.linked_documents      == b.linked_documents;
    }
    friend bool operator!=(const Part& a, const Part& b) { return !(a == b); }
};

} // namespace uniter::contract::design

#endif // PART_H
