#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "../resourceabstract.h"
#include "pdmtypes.h"
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace uniter::contract {
namespace design { class Assembly; }
}

namespace uniter::contract::pdm {

class Delta;        // fwd
class Diagnostic;   // fwd

/**
 * @brief Ресурс подсистемы PDM — зафиксированный срез проекта.
 *
 * Соответствует таблице `pdm_snapshot` (см. docs/db/pdm.md).
 *
 * Snapshot — неизменяемый срез DESIGN в определённый момент времени.
 * Snapshot'ы связаны в двусвязный список, при этом каждая пара соседних
 * снэпшотов дополнительно связана ресурсом Delta.
 *
 * **Связь указателями (а не id).**
 * В БД все четыре связи (prev/next_snapshot, prev/next_delta,
 * root_assembly) хранятся как FK по id — это инвариант реляционной БД.
 * В РАНТАЙМ-классе связи выражены через `std::shared_ptr<...>`: так
 * бизнес-логика (например, итерирование по дельтам с применением
 * изменений в PDMManager) может переходить от объекта к объекту без
 * повторных запросов к DataManager. Сам `id` по-прежнему доступен
 * у указуемого объекта (у него своё поле `ResourceAbstract::id`),
 * и именно этот id попадает в БД как FK.
 *
 * root_assembly указывает на ЗАМОРОЖЕННУЮ копию корневой сборки
 * (design::Assembly с ResourceType::ASSEMBLY_PDM — класс тот же, но
 * лежит в pdm_assembly, см. docs/db/pdm.md). Это гарантирует, что любые
 * правки в design_* не нарушат консистентность снэпшота.
 *
 * Диагностика парсинга (ошибки / замечания / info) хранится как вектор
 * `std::shared_ptr<Diagnostic>`. Сам Diagnostic — отдельный ресурс PDM
 * (ResourceType::DIAGNOSTIC); связь N:M с Snapshot лежит в join-таблице
 * `pdm_snapshot_diagnostics` (см. docs/db/pdm.md).
 *
 * Механизм создания snapshot:
 *   1. PDMManager парсит PDF проекта.
 *   2. Создаёт новый pdm_snapshot.
 *   3. Копирует все актуальные design_assembly/design_part/design_assembly_config/
 *      design_part_config (и все join-таблицы) в соответствующие pdm_* таблицы,
 *      проставляя им FK snapshot_id.
 *   4. Выставляет pdm_snapshot.root_assembly в указатель на замороженный
 *      pdm_assembly.
 *   5. Обновляет pdm_project.head_snapshot на новый snapshot.
 *   6. Создаёт pdm_delta между предыдущим head и новым head, связывает
 *      prev/next_delta у обоих снэпшотов.
 */
class Snapshot : public ResourceAbstract {
public:
    Snapshot()
        : ResourceAbstract(
              Subsystem::PDM,
              GenSubsystemType::NOTGEN,
              ResourceType::SNAPSHOT) {}
    Snapshot(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        std::shared_ptr<design::Assembly> root_assembly_ = nullptr,
        std::shared_ptr<Snapshot>         prev_snapshot_ = nullptr,
        std::shared_ptr<Snapshot>         next_snapshot_ = nullptr,
        std::shared_ptr<Delta>            prev_delta_    = nullptr,
        std::shared_ptr<Delta>            next_delta_    = nullptr)
        : ResourceAbstract(
              Subsystem::PDM,
              GenSubsystemType::NOTGEN,
              ResourceType::SNAPSHOT,
              s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , root_assembly (std::move(root_assembly_))
        , prev_snapshot (std::move(prev_snapshot_))
        , next_snapshot (std::move(next_snapshot_))
        , prev_delta    (std::move(prev_delta_))
        , next_delta    (std::move(next_delta_)) {}

    // Корневая сборка снэпшота — указатель на ЗАМОРОЖЕННУЮ копию
    // (design::Assembly, но с ResourceType::ASSEMBLY_PDM; лежит в pdm_assembly).
    // Это инвариант неизменности снэпшота. В БД — FK pdm_assembly.id.
    std::shared_ptr<design::Assembly> root_assembly;

    // Двусторонний связный список снэпшотов (версионная цепочка).
    // В БД — FK → pdm_snapshot.id.
    std::shared_ptr<Snapshot> prev_snapshot;
    std::shared_ptr<Snapshot> next_snapshot;

    // Дублирующие ссылки на смежные дельты — для быстрой навигации.
    // prev_delta = дельта, приведшая в этот снэпшот (от prev_snapshot);
    // next_delta = дельта, уходящая из этого снэпшота (в next_snapshot).
    // В БД — FK → pdm_delta.id.
    std::shared_ptr<Delta> prev_delta;
    std::shared_ptr<Delta> next_delta;

    // Диагностика парсинга (ошибки, предупреждения, info-сообщения).
    // Каждый Diagnostic — отдельный ресурс PDM (ResourceType::DIAGNOSTIC),
    // связь N:M с Snapshot в БД через join-таблицу `pdm_snapshot_diagnostics`.
    // В рантайме вектор материализуется при загрузке снэпшота. Старые поля
    // error_count/warning_count удалены: при необходимости они вычисляются
    // по этому вектору (std::count_if по severity).
    std::vector<std::shared_ptr<Diagnostic>> diagnostics;

    // ----------------------------------------------------------------------
    // Прикладные поля (TODO: подтвердить необходимость при реализации
    // PDMManager). Структурная схема PDM.pdf их не требует, но рабочий
    // цикл снэпшота (DRAFT → APPROVED → ARCHIVED) и отчёты о валидации
    // ЕСКД без них неудобны. Оставлены как TODO на подумать.
    // ----------------------------------------------------------------------

    // Версия снэпшота в пределах pdm_project. Монотонно растёт.
    uint32_t version = 0;

    // Жизненный цикл (DRAFT/APPROVED/ARCHIVED).
    SnapshotStatus status = SnapshotStatus::DRAFT;

    // Метаданные утверждения — заполняются при переходе DRAFT → APPROVED
    std::optional<uint64_t>  approved_by;
    std::optional<QDateTime> approved_at;

    friend bool operator==(const Snapshot& a, const Snapshot& b) {
        // Указатели сравниваем по адресу: равенство структуры двух снэпшотов
        // определяется тем, что они ссылаются на одни и те же объекты в
        // графе ресурсов (именно так DataManager их и восстанавливает по
        // id при десериализации). Глубокое сравнение графа вызвало бы
        // рекурсию в двусвязном списке.
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.root_assembly.get() == b.root_assembly.get()
            && a.prev_snapshot.get() == b.prev_snapshot.get()
            && a.next_snapshot.get() == b.next_snapshot.get()
            && a.prev_delta.get()    == b.prev_delta.get()
            && a.next_delta.get()    == b.next_delta.get()
            && a.diagnostics         == b.diagnostics
            && a.version             == b.version
            && a.status              == b.status
            && a.approved_by         == b.approved_by
            && a.approved_at         == b.approved_at;
    }
    friend bool operator!=(const Snapshot& a, const Snapshot& b) { return !(a == b); }
};

} // namespace uniter::contract::pdm

#endif // SNAPSHOT_H
