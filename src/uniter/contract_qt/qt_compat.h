#ifndef UNITER_CONTRACT_QT_COMPAT_H
#define UNITER_CONTRACT_QT_COMPAT_H

/**
 * @file qt_compat.h
 * @brief Адаптеры между std-типами контракта и Qt-типами клиентского кода.
 *
 * Контракт (uniter-contract) собирается без Qt и использует
 * std::string / std::chrono::system_clock::time_point / std::map<...>.
 * Клиент использует QString / QDateTime / QVariantMap в UI и сигналах.
 *
 * Этот заголовок предоставляет набор inline-функций-конверторов, которые
 * применяются строго на границе "контракт ↔ клиентский Qt-код":
 *
 *   - вход: пользовательский ввод из QLineEdit / QLabel → std::string →
 *     в поле ResourceAbstract-наследника.
 *   - выход: значение из ResourceAbstract → QString → в QLabel / qDebug.
 *
 * Внутренняя бизнес-логика (data/, control/) должна постепенно переходить
 * на std::string напрямую, без посредников.
 */

#include <QString>
#include <QDateTime>
#include <QDebug>

#include <chrono>
#include <map>
#include <string>

namespace uniter::qt_compat {

// ---------------------------------------------------------------------------
// std::string <-> QString
// ---------------------------------------------------------------------------

inline QString toQ(const std::string& s)            { return QString::fromStdString(s); }
inline QString toQ(std::string_view s)              { return QString::fromUtf8(s.data(), static_cast<int>(s.size())); }
inline QString toQ(const char* s)                   { return QString::fromUtf8(s ? s : ""); }

inline std::string fromQ(const QString& q)          { return q.toStdString(); }

// ---------------------------------------------------------------------------
// std::chrono::system_clock::time_point <-> QDateTime
// ---------------------------------------------------------------------------

inline QDateTime toQ(std::chrono::system_clock::time_point tp) {
    using namespace std::chrono;
    const auto ms = duration_cast<milliseconds>(tp.time_since_epoch()).count();
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(ms), Qt::UTC);
}

inline std::chrono::system_clock::time_point fromQ(const QDateTime& dt) {
    using namespace std::chrono;
    return system_clock::time_point(milliseconds(dt.toMSecsSinceEpoch()));
}

// ---------------------------------------------------------------------------
// std::map<std::string, std::string> (add_data) <-> Qt-friendly helpers
// ---------------------------------------------------------------------------

inline void addDataSet(std::map<std::string, std::string>& m,
                       const QString& key, const QString& value) {
    m[fromQ(key)] = fromQ(value);
}

inline QString addDataGet(const std::map<std::string, std::string>& m,
                          const QString& key,
                          const QString& default_value = {}) {
    const auto it = m.find(fromQ(key));
    return (it == m.end()) ? default_value : toQ(it->second);
}

// ---------------------------------------------------------------------------
// QDebug-совместимый вывод std::string
// (в Qt ≥ 5.14 уже есть, но дублируем для совместимости со старыми сборками)
// ---------------------------------------------------------------------------

inline QDebug operator<<(QDebug debug, const std::string& s) {
    return debug << QString::fromStdString(s);
}

} // namespace uniter::qt_compat

// Глобальные сокращения (опционально; использовать только в .cpp клиента).
// Намеренно не using namespace — чтобы точка подключения была явной.

#endif // UNITER_CONTRACT_QT_COMPAT_H
