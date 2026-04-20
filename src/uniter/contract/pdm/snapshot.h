#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "../resourceabstract.h"
#include "pdmtypes.h"
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <optional>

namespace uniter::contract::pdm {

/**
 * @brief Ресурс подсистемы PDM — зафиксированный срез проекта.
 *
 * Snapshot — аналог ветки git: один Project может иметь несколько Snapshot
 * (разные версии), каждый независимо проходит DRAFT → APPROVED → ARCHIVED.
 *
 * XML-содержимое Snapshot хранится в MinIO (не inline в БД), запись содержит
 * только ссылку `xml_object_key` + sha256 для валидации. Структура XML —
 * см. «Документация к проекту.txt» §9 и docs/pdm_design_architecture.md §1.5.
 */
class Snapshot : public ResourceAbstract {
public:
    Snapshot() = default;
    Snapshot(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        uint64_t project_id_,
        uint32_t version_,
        std::optional<uint64_t> previous_snapshot_id_,
        QString  xml_object_key_,
        QString  xml_sha256_,
        SnapshotStatus status_ = SnapshotStatus::DRAFT)
        : ResourceAbstract(s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , project_id           (project_id_)
        , version              (version_)
        , previous_snapshot_id (std::move(previous_snapshot_id_))
        , xml_object_key       (std::move(xml_object_key_))
        , xml_sha256           (std::move(xml_sha256_))
        , status               (status_) {}

    uint64_t project_id = 0;                        // FK → projects.id
    uint32_t version    = 0;                        // Монотонно возрастающий номер версии
    std::optional<uint64_t> previous_snapshot_id;   // FK → snapshots.id (NULL для первого)

    QString xml_object_key;   // MinIO key XML-файла снэпшота
    QString xml_sha256;       // SHA-256 XML-файла (валидация целостности)

    SnapshotStatus status = SnapshotStatus::DRAFT;

    // Метаданные утверждения — заполняются при переходе DRAFT → APPROVED
    std::optional<uint64_t>  approved_by;
    std::optional<QDateTime> approved_at;

    // Итоги валидации ЕСКД
    uint32_t error_count   = 0;
    uint32_t warning_count = 0;

    friend bool operator==(const Snapshot& a, const Snapshot& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.project_id           == b.project_id
            && a.version              == b.version
            && a.previous_snapshot_id == b.previous_snapshot_id
            && a.xml_object_key       == b.xml_object_key
            && a.xml_sha256           == b.xml_sha256
            && a.status               == b.status
            && a.approved_by          == b.approved_by
            && a.approved_at          == b.approved_at
            && a.error_count          == b.error_count
            && a.warning_count        == b.warning_count;
    }
    friend bool operator!=(const Snapshot& a, const Snapshot& b) { return !(a == b); }
};

} // namespace uniter::contract::pdm

#endif // SNAPSHOT_H
