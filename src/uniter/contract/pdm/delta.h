#ifndef DELTA_H
#define DELTA_H

#include "../resourceabstract.h"
#include "pdmtypes.h"
#include <QString>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace uniter::contract::pdm {

class Snapshot; // fwd

/**
 * @brief Стабильный ключ элемента дельты.
 *
 * Ключ составной: designation (строковое обозначение ЕСКД, стабильное
 * между версиями) + тип элемента (ASSEMBLY / ASSEMBLY_CONFIG / PART /
 * PART_CONFIG). Для конфигураций designation — это
 * "обозначение_родителя::config_code".
 *
 * Два элемента с одинаковым ключом в двух соседних снэпшотах считаются
 * «тем же самым элементом, возможно, изменившимся».
 */
struct DeltaKey {
    QString           designation;
    DeltaElementType  element_type = DeltaElementType::PART;

    friend bool operator<(const DeltaKey& a, const DeltaKey& b) {
        if (a.element_type != b.element_type) return a.element_type < b.element_type;
        return a.designation < b.designation;
    }
    friend bool operator==(const DeltaKey& a, const DeltaKey& b) {
        return a.element_type == b.element_type
            && a.designation  == b.designation;
    }
    friend bool operator!=(const DeltaKey& a, const DeltaKey& b) { return !(a == b); }
};

/**
 * @brief Ресурс подсистемы PDM — инкрементальные изменения между снапшотами.
 *
 * Соответствует таблице `pdm_delta` (см. docs/db/pdm.md).
 *
 * Delta связана одновременно с парой смежных снэпшотов (prev/next_snapshot)
 * и с парой смежных дельт (prev/next_delta) — это двусторонний связный
 * список в пределах pdm_project. В БД все четыре связи — FK по id;
 * в рантайме — `std::shared_ptr`, чтобы PDMManager мог итерировать по
 * цепочке дельт и применять изменения без дополнительных запросов.
 *
 * **Содержимое дельты.**
 *
 *   changes — std::map<DeltaKey, pair<shared_ptr<old>, shared_ptr<new>>>.
 *     Ключ (designation + element_type) идентифицирует элемент между
 *     версиями. Пара (old, new) хранит «было → стало»:
 *       - old == nullptr, new != nullptr  — элемент ДОБАВЛЕН;
 *       - old != nullptr, new != nullptr  — элемент ИЗМЕНЁН;
 *       (удаления живут в отдельном векторе `removed`).
 *     Указатели ведут на ResourceAbstract — конкретный тип
 *     (Assembly / AssemblyConfig / Part / PartConfig) определяется
 *     `ResourceAbstract::resource_type` у объекта.
 *
 *   removed — std::vector<shared_ptr<ResourceAbstract>>.
 *     Полный список элементов, исчезнувших в новой версии.
 *     Вынесен из map, потому что в pair<old,new> семантика "был, не стал"
 *     была бы (old != nullptr, new == nullptr), и это путало бы итерацию.
 *
 * При применении дельты к prev_snapshot PDMManager идёт по `changes` и
 * для каждого ключа:
 *   - если old == nullptr  → вставляет new в снэпшот;
 *   - иначе                → заменяет old на new;
 * затем удаляет всё, что в `removed`.
 *
 * Старые структуры DeltaChange / DeltaFileChange убраны: они были
 * проекцией полей таблиц pdm_delta_changes / pdm_delta_file_changes,
 * но при наличии полных old/new указателей такая детализация
 * избыточна — diff можно посчитать по паре объектов.
 */
class Delta : public ResourceAbstract {
public:
    using ChangesMap = std::map<
        DeltaKey,
        std::pair<std::shared_ptr<ResourceAbstract>,
                  std::shared_ptr<ResourceAbstract>>
    >;

    Delta()
        : ResourceAbstract(
              Subsystem::PDM,
              GenSubsystemType::NOTGEN,
              ResourceType::DELTA) {}
    Delta(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        std::shared_ptr<Snapshot> prev_snapshot_ = nullptr,
        std::shared_ptr<Snapshot> next_snapshot_ = nullptr,
        std::shared_ptr<Delta>    prev_delta_    = nullptr,
        std::shared_ptr<Delta>    next_delta_    = nullptr)
        : ResourceAbstract(
              Subsystem::PDM,
              GenSubsystemType::NOTGEN,
              ResourceType::DELTA,
              s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , prev_snapshot (std::move(prev_snapshot_))
        , next_snapshot (std::move(next_snapshot_))
        , prev_delta    (std::move(prev_delta_))
        , next_delta    (std::move(next_delta_)) {}

    // Связь со снэпшотами, между которыми лежит дельта.
    // В БД — FK → pdm_snapshot.id.
    std::shared_ptr<Snapshot> prev_snapshot;   // (ОТ)
    std::shared_ptr<Snapshot> next_snapshot;   // (ДО)

    // Двусвязный список дельт проекта. В БД — FK → pdm_delta.id.
    std::shared_ptr<Delta> prev_delta;
    std::shared_ptr<Delta> next_delta;

    // Добавленные + изменённые элементы (ключ → пара old/new).
    // Для ADD old == nullptr; для MODIFY оба ненулевые.
    ChangesMap changes;

    // Удалённые элементы (в отдельном векторе, а не в changes с new==nullptr —
    // чтобы итерирование "old→new" по changes не обрабатывало удаления).
    std::vector<std::shared_ptr<ResourceAbstract>> removed;

    friend bool operator==(const Delta& a, const Delta& b) {
        // Смежные снэпшоты и дельты сравниваются по адресу указателя
        // (глубокое сравнение вызвало бы рекурсию в двусвязном списке).
        // Содержимое changes / removed сравнивается по адресам указателей
        // внутри — DataManager восстанавливает одни и те же объекты по id.
        if (!(static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b))) return false;
        if (a.prev_snapshot.get() != b.prev_snapshot.get()) return false;
        if (a.next_snapshot.get() != b.next_snapshot.get()) return false;
        if (a.prev_delta.get()    != b.prev_delta.get())    return false;
        if (a.next_delta.get()    != b.next_delta.get())    return false;
        if (a.changes.size()  != b.changes.size())  return false;
        if (a.removed.size()  != b.removed.size())  return false;
        auto ita = a.changes.begin();
        auto itb = b.changes.begin();
        for (; ita != a.changes.end(); ++ita, ++itb) {
            if (!(ita->first == itb->first)) return false;
            if (ita->second.first.get()  != itb->second.first.get())  return false;
            if (ita->second.second.get() != itb->second.second.get()) return false;
        }
        for (size_t i = 0; i < a.removed.size(); ++i) {
            if (a.removed[i].get() != b.removed[i].get()) return false;
        }
        return true;
    }
    friend bool operator!=(const Delta& a, const Delta& b) { return !(a == b); }
};

} // namespace uniter::contract::pdm

#endif // DELTA_H
