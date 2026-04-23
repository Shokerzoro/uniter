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
#include <QTimeZone>

#include "../contract/uniterprotocol.h"

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
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(ms), QTimeZone::utc());
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

// ---------------------------------------------------------------------------
// QDebug operator<< для enum'ов из uniterprotocol.h
//
// Контракт использует std::string и std::ostream — Qt не знает об этих enum'ах.
// Операторы ниже делегируют в toString-функции контракта и живут вне
// namespace uniter::qt_compat, чтобы находиться через ADL при вызове
// qDebug() << value из любого места, где подключён этот заголовок.
// ---------------------------------------------------------------------------

inline QDebug operator<<(QDebug debug, uniter::contract::Subsystem v) {
    return debug << QString::fromStdString(uniter::contract::subsystemToString(v));
}

inline QDebug operator<<(QDebug debug, uniter::contract::GenSubsystemType v) {
    return debug << QString::fromStdString(uniter::contract::genSubsystemTypeToString(v));
}

inline QDebug operator<<(QDebug debug, uniter::contract::CrudAction v) {
    switch (v) {
    case uniter::contract::CrudAction::NOTCRUD:  return debug << "NOTCRUD";
    case uniter::contract::CrudAction::CREATE:   return debug << "CREATE";
    case uniter::contract::CrudAction::READ:     return debug << "READ";
    case uniter::contract::CrudAction::UPDATE:   return debug << "UPDATE";
    case uniter::contract::CrudAction::DELETE:   return debug << "DELETE";
    default: return debug << "CrudAction(" << static_cast<int>(v) << ")";
    }
}

inline QDebug operator<<(QDebug debug, uniter::contract::ProtocolAction v) {
    switch (v) {
    case uniter::contract::ProtocolAction::NOTPROTOCOL:             return debug << "NOTPROTOCOL";
    case uniter::contract::ProtocolAction::AUTH:                    return debug << "AUTH";
    case uniter::contract::ProtocolAction::FULL_SYNC:               return debug << "FULL_SYNC";
    case uniter::contract::ProtocolAction::GET_KAFKA_CREDENTIALS:   return debug << "GET_KAFKA_CREDENTIALS";
    case uniter::contract::ProtocolAction::GET_MINIO_PRESIGNED_URL: return debug << "GET_MINIO_PRESIGNED_URL";
    case uniter::contract::ProtocolAction::GET_MINIO_FILE:          return debug << "GET_MINIO_FILE";
    case uniter::contract::ProtocolAction::PUT_MINIO_FILE:          return debug << "PUT_MINIO_FILE";
    case uniter::contract::ProtocolAction::UPDATE_CHECK:            return debug << "UPDATE_CHECK";
    case uniter::contract::ProtocolAction::UPDATE_CONSENT:          return debug << "UPDATE_CONSENT";
    case uniter::contract::ProtocolAction::UPDATE_DOWNLOAD:         return debug << "UPDATE_DOWNLOAD";
    default: return debug << "ProtocolAction(" << static_cast<int>(v) << ")";
    }
}

inline QDebug operator<<(QDebug debug, uniter::contract::MessageStatus v) {
    switch (v) {
    case uniter::contract::MessageStatus::REQUEST:       return debug << "REQUEST";
    case uniter::contract::MessageStatus::RESPONSE:      return debug << "RESPONSE";
    case uniter::contract::MessageStatus::ERROR:         return debug << "ERROR";
    case uniter::contract::MessageStatus::NOTIFICATION:  return debug << "NOTIFICATION";
    case uniter::contract::MessageStatus::SUCCESS:       return debug << "SUCCESS";
    default: return debug << "MessageStatus(" << static_cast<int>(v) << ")";
    }
}

inline QDebug operator<<(QDebug debug, uniter::contract::ErrorCode v) {
    switch (v) {
    case uniter::contract::ErrorCode::SUCCESS:             return debug << "SUCCESS(0)";
    case uniter::contract::ErrorCode::BAD_REQUEST:         return debug << "BAD_REQUEST(400)";
    case uniter::contract::ErrorCode::BAD_TIMING:          return debug << "BAD_TIMING(401)";
    case uniter::contract::ErrorCode::PERMISSION_DENIED:   return debug << "PERMISSION_DENIED(423)";
    case uniter::contract::ErrorCode::INTERNAL_ERROR:      return debug << "INTERNAL_ERROR(500)";
    case uniter::contract::ErrorCode::SERVICE_UNAVAILABLE: return debug << "SERVICE_UNAVAILABLE(503)";
    default: return debug << "ErrorCode(" << static_cast<int>(v) << ")";
    }
}

inline QDebug operator<<(QDebug debug, uniter::contract::ResourceType v) {
    const auto i = static_cast<int>(v);
    switch (v) {
    case uniter::contract::ResourceType::DEFAULT:                      return debug << "DEFAULT";
    case uniter::contract::ResourceType::EMPLOYEES:                    return debug << "EMPLOYEES";
    case uniter::contract::ResourceType::PRODUCTION:                   return debug << "PRODUCTION";
    case uniter::contract::ResourceType::INTEGRATION:                  return debug << "INTEGRATION";
    case uniter::contract::ResourceType::MATERIAL_TEMPLATE_SIMPLE:     return debug << "MATERIAL_TEMPLATE_SIMPLE";
    case uniter::contract::ResourceType::MATERIAL_TEMPLATE_COMPOSITE:  return debug << "MATERIAL_TEMPLATE_COMPOSITE";
    case uniter::contract::ResourceType::PROJECT:                      return debug << "PROJECT";
    case uniter::contract::ResourceType::ASSEMBLY:                     return debug << "ASSEMBLY";
    case uniter::contract::ResourceType::PART:                         return debug << "PART";
    case uniter::contract::ResourceType::ASSEMBLY_CONFIG:              return debug << "ASSEMBLY_CONFIG";
    case uniter::contract::ResourceType::PART_CONFIG:                  return debug << "PART_CONFIG";
    case uniter::contract::ResourceType::PURCHASE_GROUP:               return debug << "PURCHASE_GROUP";
    case uniter::contract::ResourceType::PURCHASE:                     return debug << "PURCHASE";
    case uniter::contract::ResourceType::SNAPSHOT:                     return debug << "SNAPSHOT";
    case uniter::contract::ResourceType::DELTA:                        return debug << "DELTA";
    case uniter::contract::ResourceType::PDM_PROJECT:                  return debug << "PDM_PROJECT";
    case uniter::contract::ResourceType::DIAGNOSTIC:                   return debug << "DIAGNOSTIC";
    case uniter::contract::ResourceType::MATERIAL_INSTANCE_SIMPLE:     return debug << "MATERIAL_INSTANCE_SIMPLE";
    case uniter::contract::ResourceType::MATERIAL_INSTANCE_COMPOSITE:  return debug << "MATERIAL_INSTANCE_COMPOSITE";
    case uniter::contract::ResourceType::PRODUCTION_TASK:              return debug << "PRODUCTION_TASK";
    case uniter::contract::ResourceType::PRODUCTION_STOCK:             return debug << "PRODUCTION_STOCK";
    case uniter::contract::ResourceType::PRODUCTION_SUPPLY:            return debug << "PRODUCTION_SUPPLY";
    case uniter::contract::ResourceType::INTEGRATION_TASK:             return debug << "INTEGRATION_TASK";
    case uniter::contract::ResourceType::DOC:                          return debug << "DOC";
    case uniter::contract::ResourceType::DOC_LINK:                     return debug << "DOC_LINK";
    default: return debug << "ResourceType(" << i << ")";
    }
}

// Глобальные сокращения (опционально; использовать только в .cpp клиента).
// Намеренно не using namespace — чтобы точка подключения была явной.

#endif // UNITER_CONTRACT_QT_COMPAT_H
