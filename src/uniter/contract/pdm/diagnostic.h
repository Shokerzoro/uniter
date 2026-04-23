#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "../resourceabstract.h"
#include "pdmtypes.h"
#include <QString>
#include <cstdint>

namespace uniter::contract::pdm {

/**
 * @brief Ресурс подсистемы PDM — одна ошибка/замечание парсинга КД.
 *
 * Соответствует таблице `pdm_diagnostic` (см. docs/db/pdm.md).
 *
 * Раньше диагностика была денормализована внутри Snapshot как пара полей
 * `error_count` / `warning_count`. Теперь это полноценный ресурс со своим
 * id и ResourceType::DIAGNOSTIC (57). Один Snapshot может ссылаться на
 * N Diagnostic, один Diagnostic может принадлежать нескольким Snapshot'ам
 * (если проблема повторяется между версиями) — связь N:M через join-таблицу
 * `pdm_snapshot_diagnostics` (snapshot_id, diagnostic_id).
 *
 * В рантайм-классе Snapshot связь свёрнута в вектор
 * `std::vector<std::shared_ptr<Diagnostic>> diagnostics`; сам Diagnostic
 * своих snapshot_id не хранит — эту сторону связи восстанавливает
 * DataManager запросом по join-таблице при необходимости.
 *
 * Набор категорий и типов диагностики отражает XML-секцию
 * `<Errors><Error severity="" category="" type="" path=""/>` в snapshot'е.
 * См. skills/user/uniter-target-state и docs/db/pdm.md.
 */
class Diagnostic : public ResourceAbstract {
public:
    Diagnostic()
        : ResourceAbstract(
              Subsystem::PDM,
              GenSubsystemType::NOTGEN,
              ResourceType::DIAGNOSTIC) {}
    Diagnostic(
        uint64_t s_id,
        bool actual,
        const QDateTime& c_created_at,
        const QDateTime& s_updated_at,
        uint64_t s_created_by,
        uint64_t s_updated_by,
        DiagnosticSeverity severity_,
        DiagnosticCategory category_,
        QString type_,
        QString path_,
        QString message_ = QString())
        : ResourceAbstract(
              Subsystem::PDM,
              GenSubsystemType::NOTGEN,
              ResourceType::DIAGNOSTIC,
              s_id, actual, c_created_at, s_updated_at, s_created_by, s_updated_by)
        , severity (severity_)
        , category (category_)
        , type     (std::move(type_))
        , path     (std::move(path_))
        , message  (std::move(message_)) {}

    // Уровень серьёзности (ERROR / WARNING / INFO).
    DiagnosticSeverity severity = DiagnosticSeverity::ERROR;

    // Крупная категория (FILE_SYSTEM, VERSION_CONTROL, PARSING, VALIDATION…).
    DiagnosticCategory category = DiagnosticCategory::PARSING;

    // Конкретный код/тип диагностики (строковый идентификатор, например
    // "NO_FILE", "INFORMAL_CHANGE"). Строка — чтобы не разрастать enum
    // при добавлении новых видов; каноничный перечень — в docs/db/pdm.md.
    QString type;

    // Путь / адрес объекта, к которому относится диагностика: файл PDF,
    // XPath внутри snapshot'а, designation детали и т.п.
    QString path;

    // Развёрнутое человекочитаемое сообщение (опционально).
    QString message;

    friend bool operator==(const Diagnostic& a, const Diagnostic& b) {
        return static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
            && a.severity == b.severity
            && a.category == b.category
            && a.type     == b.type
            && a.path     == b.path
            && a.message  == b.message;
    }
    friend bool operator!=(const Diagnostic& a, const Diagnostic& b) { return !(a == b); }
};

} // namespace uniter::contract::pdm

#endif // DIAGNOSTIC_H
