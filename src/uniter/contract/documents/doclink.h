#ifndef DOCLINK_H
#define DOCLINK_H

#include "../resourceabstract.h"
#include "doctypes.h"

#include <tinyxml2.h>
#include <QString>
#include <cstdint>

namespace uniter::contract::documents {

/**
 * @brief Ресурс подсистемы DOCUMENTS — привязка Doc к конкретному ресурсу.
 *
 * DocLink — это явная N:M-связь между документом (Doc) и другим ресурсом
 * (Assembly, Part, Project, MaterialTemplate*). Сделан отдельным ресурсом
 * (а не вложенной структурой) по следующим причинам:
 *   - CRUD-операции над связью (прикрепить/открепить) становятся
 *     обычными CRUD-сообщениями UniterMessage;
 *   - разрешения на прикрепление документов управляются независимо
 *     от разрешений на сам документ и на целевой ресурс;
 *   - запрос вида «все документы сборки X» = SELECT из одной таблицы.
 *
 * Реляционная таблица `doc_links`:
 *   id            PK
 *   doc_id        INTEGER    FK → docs.id
 *   target_type   INTEGER    (DocLinkTargetType)
 *   target_id     INTEGER    FK → <target_type-специфичная таблица>.id
 *   role          TEXT       (роль документа в контексте target; опционально)
 *   position      INTEGER    (порядковый номер — для упорядоченных списков)
 *   + общие поля ResourceAbstract
 *
 * Про `role`: в некоторых случаях нужно различать «первичный чертёж» и
 * «вспомогательный», «PDF ГОСТа» и «выдержка из ТУ» и т.п. — для этого
 * служит строковое поле `role`. Для большинства случаев оно пустое.
 *
 * ВАЖНО: по принципу реляционной БД внутри DocLink НЕ хранится сам Doc
 * (только doc_id). Загрузка конкретного Doc выполняется отдельным
 * запросом к DataManager.
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
        uint64_t doc_id_,
        DocLinkTargetType target_type_,
        uint64_t target_id_,
        QString  role_     = QString(),
        uint32_t position_ = 0)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , doc_id      (doc_id_)
        , target_type (target_type_)
        , target_id   (target_id_)
        , role        (std::move(role_))
        , position    (position_) {}

    // FK на сам документ
    uint64_t doc_id = 0;

    // Кому именно привязан документ (полиморфный FK: target_type + target_id)
    DocLinkTargetType target_type = DocLinkTargetType::UNKNOWN;
    uint64_t          target_id   = 0;

    // Семантика привязки (опционально — для редких случаев, см. комментарий)
    QString  role;
    uint32_t position = 0;

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml  (tinyxml2::XMLElement* dest)   override;

    friend bool operator==(const DocLink& a, const DocLink& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.doc_id      == b.doc_id
            && a.target_type == b.target_type
            && a.target_id   == b.target_id
            && a.role        == b.role
            && a.position    == b.position;
    }
    friend bool operator!=(const DocLink& a, const DocLink& b) { return !(a == b); }
};


} // namespace uniter::contract::documents

#endif // DOCLINK_H
