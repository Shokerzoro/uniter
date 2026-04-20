#ifndef RESOURCEABSTRACT_H
#define RESOURCEABSTRACT_H

#include <cstdint>
#include <QString>
#include <QDateTime>

namespace uniter {
namespace contract {

/**
 * @brief Базовый класс всех ресурсов системы.
 *
 * Главный инвариант:
 *
 *   **Ориентация на реляционную БД.**
 *   Поля ресурса спроектированы как колонки одной строки таблицы. Связи с
 *   другими ресурсами хранятся через `uint64_t id` (FK), а не через
 *   `shared_ptr` — это упрощает маппинг на SQLite и исключает eager load.
 *
 * Сериализация/десериализация ресурсов (в XML, JSON, бинарный формат
 * или SQL-строку) вынесена из ресурса и будет реализована на уровне
 * DataManager / сетевого слоя. Ресурсы — это только данные плюс
 * пополевное сравнение для тестов и observer-диффов.
 */
class ResourceAbstract
{
public:
    ResourceAbstract() = default;
    ResourceAbstract(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by)
        : id(s_id)
        , is_actual(actual)
        , created_at(c_created_at)
        , updated_at(s_updated_at)
        , created_by(s_created_by)
        , updated_by(s_updated_by)
    {}
    virtual ~ResourceAbstract() = default;

    // Общие поля всех ресурсов (колонки, наследуемые каждой таблицей SQLite)
    uint64_t  id         = 0;     // Серверный глобальный ID (PK)
    bool      is_actual  = true;  // Soft-delete флаг (actual)
    QDateTime created_at;
    QDateTime updated_at;
    uint64_t  created_by = 0;     // FK → employees.id
    uint64_t  updated_by = 0;     // FK → employees.id

    // Пополевное сравнение базовых полей ресурса. Наследники
    // вызывают этот оператор в своём operator==, чтобы не дублировать
    // перечисление id/is_actual/created_at/updated_at/created_by/updated_by.
    friend bool operator==(const ResourceAbstract& a, const ResourceAbstract& b) {
        return a.id         == b.id
            && a.is_actual  == b.is_actual
            && a.created_at == b.created_at
            && a.updated_at == b.updated_at
            && a.created_by == b.created_by
            && a.updated_by == b.updated_by;
    }
    friend bool operator!=(const ResourceAbstract& a, const ResourceAbstract& b) { return !(a == b); }
};


} // contract
} // uniter


#endif // RESOURCEABSTRACT_H
