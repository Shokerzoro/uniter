#ifndef DOC_H
#define DOC_H

#include "../resourceabstract.h"
#include "doctypes.h"

#include <tinyxml2.h>
#include <QString>
#include <cstdint>

namespace uniter::contract::documents {

/**
 * @brief Ресурс подсистемы DOCUMENTS — файл, хранящийся в MinIO.
 *
 * Doc инкапсулирует один файл (PDF чертежа, 3D-модель, PDF ГОСТа и т.п.)
 * и его метаданные. Важный принцип: Doc НЕ знает, к каким ресурсам он
 * привязан — привязки хранятся в отдельной таблице `doc_links` через
 * ресурс DocLink. Это позволяет:
 *   - переиспользовать один Doc между разными ресурсами (например,
 *     PDF одного ГОСТа используется в десятках MaterialTemplate);
 *   - удалять/архивировать документ независимо от его использования;
 *   - строить отчёты вида «где используется этот файл».
 *
 * Реляционная таблица `docs`:
 *   id             PK
 *   type           INTEGER    (DocumentType)
 *   name           TEXT       (человекочитаемое имя, напр. "ГОСТ 8239-89")
 *   object_key     TEXT       (ключ объекта в MinIO)
 *   sha256         TEXT       (хеш содержимого, для кэширования и идемпотентности)
 *   size_bytes     INTEGER    (размер файла в байтах, денормализация для UI)
 *   mime_type      TEXT       (например "application/pdf")
 *   description    TEXT       (комментарий — опционально)
 *   + общие поля ResourceAbstract (id/actual/created_at/updated_at/...)
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
        DocumentType type_,
        QString  name_,
        QString  object_key_,
        QString  sha256_,
        uint64_t size_bytes_ = 0,
        QString  mime_type_  = QString(),
        QString  description_ = QString())
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , type        (type_)
        , name        (std::move(name_))
        , object_key  (std::move(object_key_))
        , sha256      (std::move(sha256_))
        , size_bytes  (size_bytes_)
        , mime_type   (std::move(mime_type_))
        , description (std::move(description_)) {}

    // Тип документа (влияет на валидацию и отображение, см. DocumentType)
    DocumentType type = DocumentType::UNKNOWN;

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

    // Каскадная сериализация
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml  (tinyxml2::XMLElement* dest)   override;
};


} // namespace uniter::contract::documents

#endif // DOC_H
