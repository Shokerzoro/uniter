#ifndef RESOURCEABSTRACT_H
#define RESOURCEABSTRACT_H

#include <tinyxml2.h>
#include <cstdint>
#include <QString>
#include <QDateTime>
#include <optional>

namespace uniter {
namespace contract {

/**
 * @brief Базовый класс всех ресурсов системы.
 *
 * Две главные инвариантные идеи:
 *
 * 1. **Каскадная сериализация в XML.**
 *    Каждый наследник обязан в самом начале своих реализаций `to_xml()` / `from_xml()`
 *    вызвать `ResourceAbstract::to_xml(dest)` / `ResourceAbstract::from_xml(source)`
 *    — это гарантирует, что общие поля ресурса (id, actual, created_at, updated_at,
 *    created_by, updated_by) будут записаны/прочитаны автоматически, а наследник
 *    добавит свои специфичные поля поверх.
 *
 * 2. **Ориентация на реляционную БД.**
 *    Поля ресурса спроектированы как колонки одной строки таблицы. Связи с
 *    другими ресурсами хранятся через `uint64_t id` (FK), а не через
 *    `shared_ptr` — это упрощает маппинг на SQLite и исключает eager load.
 *
 * Служебные хелперы `put*` / `get*` нормализуют работу с tinyxml2 (атрибуты
 * сохраняются как UTF-8 строки; QDateTime сериализуется в ISO 8601 UTC).
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

    // Каскадная сериализация. Наследники должны вызывать
    //   ResourceAbstract::to_xml(dest);
    //   ResourceAbstract::from_xml(source);
    // ПЕРВОЙ строкой своих реализаций.
    virtual void to_xml(tinyxml2::XMLElement* dest);
    virtual void from_xml(tinyxml2::XMLElement* source);

    // -------- Хелперы для tinyxml2 (доступны наследникам) --------

    // Запись атрибутов
    static void putString  (tinyxml2::XMLElement* el, const char* name, const QString& v);
    static void putUInt64  (tinyxml2::XMLElement* el, const char* name, uint64_t v);
    static void putUInt32  (tinyxml2::XMLElement* el, const char* name, uint32_t v);
    static void putInt     (tinyxml2::XMLElement* el, const char* name, int v);
    static void putBool    (tinyxml2::XMLElement* el, const char* name, bool v);
    static void putDouble  (tinyxml2::XMLElement* el, const char* name, double v);
    static void putDateTime(tinyxml2::XMLElement* el, const char* name, const QDateTime& v);
    static void putOptUInt64  (tinyxml2::XMLElement* el, const char* name, const std::optional<uint64_t>& v);
    static void putOptDateTime(tinyxml2::XMLElement* el, const char* name, const std::optional<QDateTime>& v);

    // Чтение атрибутов
    static QString   getString  (const tinyxml2::XMLElement* el, const char* name, const QString& def = QString());
    static uint64_t  getUInt64  (const tinyxml2::XMLElement* el, const char* name, uint64_t def = 0);
    static uint32_t  getUInt32  (const tinyxml2::XMLElement* el, const char* name, uint32_t def = 0);
    static int       getInt     (const tinyxml2::XMLElement* el, const char* name, int def = 0);
    static bool      getBool    (const tinyxml2::XMLElement* el, const char* name, bool def = false);
    static double    getDouble  (const tinyxml2::XMLElement* el, const char* name, double def = 0.0);
    static QDateTime getDateTime(const tinyxml2::XMLElement* el, const char* name);
    static std::optional<uint64_t>  getOptUInt64  (const tinyxml2::XMLElement* el, const char* name);
    static std::optional<QDateTime> getOptDateTime(const tinyxml2::XMLElement* el, const char* name);
};


} // contract
} // uniter


#endif // RESOURCEABSTRACT_H
