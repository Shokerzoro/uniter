#ifndef UNITER_COMPATIBILITY_QT_COMPAT_H
#define UNITER_COMPATIBILITY_QT_COMPAT_H

#include <chrono>
#include <string>

#ifdef QT_CORE_LIB
#include <QDateTime>
#include <QString>
#endif

namespace uniter::compatibility {

#ifdef QT_CORE_LIB
inline std::chrono::system_clock::time_point qDateTimeToTimePoint(const QDateTime& value)
{
    if (!value.isValid()) {
        return {};
    }
    return std::chrono::system_clock::time_point(
        std::chrono::milliseconds(value.toMSecsSinceEpoch()));
}

inline std::string qStringToStdString(const QString& value)
{
    return value.toStdString();
}

inline QString stdStringToQString(const std::string& value)
{
    return QString::fromStdString(value);
}

inline QDateTime timePointToQDateTime(const std::chrono::system_clock::time_point& value)
{
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        value.time_since_epoch()).count();
    return QDateTime::fromMSecsSinceEpoch(ms);
}
#endif

} // namespace uniter::compatibility

#endif // UNITER_COMPATIBILITY_QT_COMPAT_H
