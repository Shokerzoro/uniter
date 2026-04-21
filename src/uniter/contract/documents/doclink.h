#ifndef DOCLINK_H
#define DOCLINK_H

#include "../resourceabstract.h"
#include "doctypes.h"
#include "doc.h"
#include <cstdint>
#include <vector>

namespace uniter::contract::documents {

/**
 * @brief Папка документов, прикреплённая к одному целевому ресурсу.
 *
 * DocLink — это «папка», к которой привязано N документов (Doc). Один
 * Doc принадлежит ровно одному DocLink (отношение 1:M); если тот же
 * физический файл нужен в другой папке, создаётся ещё один Doc с тем
 * же `object_key`/`sha256`.
 *
 * DocLink прикреплён к целевому ресурсу через **обратную** ссылку: на
 * стороне владельца (Assembly, Part, Project, MaterialTemplate*)
 * хранится `doc_link_id` — FK на DocLink. Сам DocLink не знает ни id
 * цели, ни роли/позиции документов внутри — у него только тип цели
 * (`DocLinkTargetType`) и список своих Doc.
 *
 * Соответствие БД (см. docs/db/documents.md, схема DOCUMENTS.pdf):
 *
 *   documents/doc_link (ResourceType::DOC_LINK)
 *     id                    PK
 *     doc_link_target_type  INTEGER  (DocLinkTargetType)
 *     + общие поля ResourceAbstract
 *
 * Вектор `docs` — это **свёрнутая** выборка из таблицы `documents/doc`
 * по `doc_link_id = this.id` (FK живёт только в БД, в рантайм-классе
 * Doc его нет). DataManager при загрузке DocLink достаёт
 * соответствующие Doc и материализует вектор. При изменении состава
 * папки CRUD-сообщения отправляются на сами Doc (ResourceType::DOC),
 * а не на DocLink.
 */
class DocLink : public ResourceAbstract {
public:
    DocLink() = default;
    DocLink(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        DocLinkTargetType doc_link_target_type_,
        std::vector<Doc> docs_ = {})
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , doc_link_target_type (doc_link_target_type_)
        , docs                 (std::move(docs_)) {}

    // Тип целевого ресурса, к которому прикреплена папка (см. DocLinkTargetType).
    // Сам target_id хранит владелец: у него поле `doc_link_id` указывает на эту DocLink.
    DocLinkTargetType doc_link_target_type = DocLinkTargetType::UNKNOWN;

    // Свёрнутый список документов папки.
    // Физически хранится в отдельной таблице `documents/doc` с FK `doc_link_id`;
    // здесь материализуется при чтении DocLink. Редактирование состава
    // выполняется CRUD-сообщениями на отдельные Doc, а не на всю папку.
    std::vector<Doc> docs;

    friend bool operator==(const DocLink& a, const DocLink& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.doc_link_target_type == b.doc_link_target_type
            && a.docs                 == b.docs;
    }
    friend bool operator!=(const DocLink& a, const DocLink& b) { return !(a == b); }
};

} // namespace uniter::contract::documents

#endif // DOCLINK_H
