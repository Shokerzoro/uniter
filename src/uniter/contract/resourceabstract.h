#ifndef RESOURCEABSTRACT_H
#define RESOURCEABSTRACT_H

#include <cstdint>
#include <QString>
#include <QDateTime>
#include "uniterprotocol.h"

namespace uniter {
namespace contract {

/**
 * @brief Базовый класс всех ресурсов системы.
 *
 * Главный инвариант:
 *
 *   **Ориентация на реляционную БД.**
 *   Поля ресурса спроектированы как колонки одной строки таблицы. Связи с
 *   другими ресурсами, как правило, хранятся через `uint64_t id` (FK), что
 *   упрощает маппинг на SQLite и исключает eager load. Исключения — связи,
 *   где удобнее работать с полным объектом (например, Snapshot↔Delta цепочка
 *   в PDM): там используется `std::shared_ptr<>`, но у указуемого ресурса
 *   всё равно есть собственный `id`, который и попадает в БД как FK.
 *
 * Сериализация/десериализация ресурсов (в XML, JSON, бинарный формат
 * или SQL-строку) вынесена из ресурса и будет реализована на уровне
 * DataManager / сетевого слоя. Ресурсы — это только данные плюс
 * пополевное сравнение для тестов и observer-диффов.
 *
 * Метаданные типа ресурса (`subsystem`, `gen_subsystem`, `resource_type`)
 * заполняются конструктором каждого наследника автоматически — так любой
 * ресурс, созданный `new`, фабрикой или десериализацией, сразу знает, к
 * какой подсистеме и какому ResourceType он относится. Это нужно для:
 *   - маршрутизации CRUD-сообщений в DataManager по `resource_type`;
 *   - проверки согласованности `UniterMessage::resource_type` и типа
 *     фактически приложенного ресурса;
 *   - логирования и диагностики (`qDebug() << resource`).
 *
 * Каждый наследник ОБЯЗАН передавать свои три метаданных-параметра в
 * конструктор базы (default и полный). Метаданные типа НЕ участвуют в
 * пополевном сравнении (`operator==`): они однозначно определяются
 * классом-наследником и всегда совпадают у двух объектов одного типа.
 */
class ResourceAbstract
{
public:
    /**
     * @brief Конструктор только для метаданных типа.
     *
     * Используется наследниками в своих default-конструкторах, чтобы
     * получить объект с правильно заполненными subsystem/gen_subsystem/
     * resource_type, но "пустыми" данными (id=0 и т.п.).
     */
    ResourceAbstract(
        Subsystem        s_subsystem,
        GenSubsystemType s_gen_subsystem,
        ResourceType     s_resource_type)
        : subsystem     (s_subsystem)
        , gen_subsystem (s_gen_subsystem)
        , resource_type (s_resource_type)
    {}

    /**
     * @brief Полный конструктор.
     *
     * Наследники ОБЯЗАНЫ передавать сюда свои значения `subsystem`,
     * `gen_subsystem`, `resource_type` — это и есть механизм автозаполнения.
     */
    ResourceAbstract(
        Subsystem        s_subsystem,
        GenSubsystemType s_gen_subsystem,
        ResourceType     s_resource_type,
        uint64_t         s_id,
        bool             actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t         s_created_by,
        uint64_t         s_updated_by)
        : subsystem     (s_subsystem)
        , gen_subsystem (s_gen_subsystem)
        , resource_type (s_resource_type)
        , id            (s_id)
        , is_actual     (actual)
        , created_at    (c_created_at)
        , updated_at    (s_updated_at)
        , created_by    (s_created_by)
        , updated_by    (s_updated_by)
    {}

    // Default-конструктор разрешён: используется DataManagerом для
    // десериализации (поля заполняются пошагово). Метаданные типа
    // остаются со значениями по умолчанию (PROTOCOL/NOTGEN/DEFAULT);
    // конкретные наследники ОБЯЗАНЫ переопределить их в своём default-кторе.
    ResourceAbstract() = default;
    virtual ~ResourceAbstract() = default;

    // ----------------------------------------------------------------------
    // Метаданные типа ресурса (заполняются конструктором наследника).
    // В БД не хранятся: определяются таблицей, в которой лежит строка.
    // ----------------------------------------------------------------------
    Subsystem        subsystem     = Subsystem::PROTOCOL;
    GenSubsystemType gen_subsystem = GenSubsystemType::NOTGEN;
    ResourceType     resource_type = ResourceType::DEFAULT;

    // ----------------------------------------------------------------------
    // Общие поля всех ресурсов (колонки, наследуемые каждой таблицей SQLite).
    // ----------------------------------------------------------------------
    uint64_t  id         = 0;     // Серверный глобальный ID (PK)
    bool      is_actual  = true;  // Soft-delete флаг (actual)
    QDateTime created_at;
    QDateTime updated_at;
    uint64_t  created_by = 0;     // FK → employees.id
    uint64_t  updated_by = 0;     // FK → employees.id

    // Пополевное сравнение базовых полей ресурса. Наследники
    // вызывают этот оператор в своём operator==, чтобы не дублировать
    // перечисление id/is_actual/created_at/updated_at/created_by/updated_by.
    // Метаданные типа (subsystem/gen_subsystem/resource_type) НЕ сравниваются:
    // они однозначно определяются классом-наследником.
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
