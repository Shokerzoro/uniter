#ifndef DOC_H
#define DOC_H

#include "../resourceabstract.h"
#include "doctypes.h"
#include <QString>
#include <cstdint>
#include <optional>

namespace uniter::contract::documents {

/**
 * @brief Ресурс подсистемы DOCUMENTS — файл, хранящийся в MinIO.
 *
 * Doc инкапсулирует один файл (PDF чертежа, 3D-модель, PDF ГОСТа и т.п.)
 * и его метаданные. В рантайме у Doc НЕТ полей-ссылок на другие
 * сущности: связка с DocLink выполняется со стороны DocLink через его
 * вектор `docs` (сворачивание таблиц doc_link + doc → один класс
 * DocLink). Ссылка в БД (FK `doc_link_id` на стороне `documents/doc`)
 * существует только на уровне реляционного хранения — см.
 * `docs/db/documents.md`.
 *
 * Соответствие БД (схема DOCUMENTS.pdf):
 *
 *   documents/doc (ResourceType::DOC)
 *     id              PK
 *     doc_link_id     FK → documents/doc_link.id  (только в БД, не в рантайме)
 *     doc_type        INTEGER    (DocumentType)
 *     name            TEXT       (человекочитаемое имя)
 *     object_key      TEXT       (ключ объекта в MinIO)
 *     sha256          TEXT       (SHA-256 содержимого)
 *     size_bytes      INTEGER    (размер файла, денормализация для UI)
 *     mime_type       TEXT       (например "application/pdf")
 *     description     TEXT       (комментарий)
 *     local_path      TEXT NULL  (путь в локальном кэше клиента)
 *     + общие поля ResourceAbstract
 */
class Doc : public ResourceAbstract {
public:
    Doc() = default;
    Doc(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        DocumentType doc_type_,
        QString  name_,
        QString  object_key_,
        QString  sha256_,
        uint64_t size_bytes_ = 0,
        QString  mime_type_  = QString(),
        QString  description_ = QString(),
        std::optional<QString> local_path_ = std::nullopt)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , doc_type    (doc_type_)
        , name        (std::move(name_))
        , object_key  (std::move(object_key_))
        , sha256      (std::move(sha256_))
        , size_bytes  (size_bytes_)
        , mime_type   (std::move(mime_type_))
        , description (std::move(description_))
        , local_path  (std::move(local_path_)) {}

    // Тип документа (влияет на валидацию и отображение, см. DocumentType)
    DocumentType doc_type = DocumentType::UNKNOWN;

    // Отображаемое имя (для UI; не обязано совпадать с именем файла в MinIO)
    QString name;

    // Расположение файла в MinIO и его целостность
    QString object_key;   // Ключ объекта (идентификатор в MinIO)
    QString sha256;       // SHA-256 содержимого (для кэширования и проверки)

    // Денормализованные метаданные файла
    uint64_t size_bytes = 0;
    QString  mime_type;

    // Пользовательский комментарий
    QString description;

    // Локальный путь на диске клиента (кэш MinIO).
    // Пустое optional — файл ещё не скачан или клиент решил его не хранить.
    // Это поле клиентское: сервер его не валидирует и может прислать nullopt.
    std::optional<QString> local_path;

    friend bool operator==(const Doc& a, const Doc& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.doc_type    == b.doc_type
            && a.name        == b.name
            && a.object_key  == b.object_key
            && a.sha256      == b.sha256
            && a.size_bytes  == b.size_bytes
            && a.mime_type   == b.mime_type
            && a.description == b.description
            && a.local_path  == b.local_path;
    }
    friend bool operator!=(const Doc& a, const Doc& b) { return !(a == b); }
};

} // namespace uniter::contract::documents

#endif // DOC_H
