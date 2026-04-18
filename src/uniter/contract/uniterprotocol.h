#ifndef UNITERPROTOCOL_H
#define UNITERPROTOCOL_H

#include <cstdint>
#include <QDebug>

namespace uniter::contract {

enum class Subsystem : uint8_t {
    PROTOCOL               = 0,
    MATERIALS              = 1,
    DESIGN                 = 2,
    PURCHASES              = 3,
    MANAGER                = 4,
    GENERATIVE             = 5,
    INSTANCES              = 6,
    PDM                    = 7
};

enum class GenSubsystemType : uint8_t {
    NOTGEN                 = 0,
    PRODUCTION             = 1,
    INTERGATION            = 2,
};

enum class ResourceType : uint8_t {
    // --- Общие ---
    DEFAULT,

    // --- MANAGER ---
    EMPLOYEES,

    // --- GENERATIVE ---
    PRODUCTION,
    INTEGRATION,
    GROUP,

    // --- PURCHASES ---
    PURCHASE,

    // --- DESIGN ---
    PROJECT,
    ASSEMBLY,
    PART,

    // --- PDM ---
    SNAPSHOT,
    DELTA,

    // --- INSTANCES ---
    MATERIAL_INSTANCE
};

enum class CrudAction : uint8_t {
    NOTCRUD                = 0,
    CREATE                 = 1,
    READ                   = 2,
    UPDATE                 = 3,
    DELETE                 = 4
};

// Все протокольные действия находятся в домене Subsystem::PROTOCOL
enum class ProtocolAction : uint8_t {
    NOTPROTOCOL            = 0,
    // Аутентификация
    AUTH                   = 1,
    // Полная синхронизация (POLL): сервер присылает все CRUD-события подряд
    FULL_SYNC              = 2,
    // Запрос credentials для подключения к топику Kafka
    GET_KAFKA_CREDENTIALS  = 3,
    // Запрос presigned URL у основного сервера для доступа к объекту MinIO
    GET_MINIO_PRESIGNED_URL = 4,
    // Запрос к MinIOConnector для получения (скачивания) файла по presigned URL
    GET_MINIO_FILE         = 5,
    // Поддержка обновлений: запрос наличия новой версии приложения
    UPDATE_CHECK           = 6,
    // Поддержка обновлений: согласие/отказ от обновления (add_data: {"accepted": "true"|"false"})
    UPDATE_CONSENT         = 7,
    // Поддержка обновлений: получение файлов обновления (presigned URL от сервера)
    UPDATE_DOWNLOAD        = 8,
};

enum class MessageStatus : uint8_t {
    REQUEST                = 0,
    RESPONSE               = 1,
    ERROR                  = 2,
    NOTIFICATION           = 3,
    // Подтверждение успешного выполнения собственной CUD-операции (из Kafka)
    SUCCESS                = 4
};

enum class ErrorCode : uint16_t {
    SUCCESS                = 0,
    BAD_REQUEST            = 400,
    BAD_TIMING             = 401,
    PERMISSION_DENIED      = 423,
    INTERNAL_ERROR         = 500,
    SERVICE_UNAVAILABLE    = 503,
};

// Функции преобразования enum в QString
inline QString subsystemToString(Subsystem subsystem) {
    switch(subsystem) {
    case Subsystem::PROTOCOL:    return "PROTOCOL";
    case Subsystem::MATERIALS:   return "MATERIALS";
    case Subsystem::DESIGN:      return "DESIGN";
    case Subsystem::PURCHASES:   return "PURCHASES";
    case Subsystem::MANAGER:     return "MANAGER";
    case Subsystem::GENERATIVE:  return "GENERATIVE";
    case Subsystem::INSTANCES:   return "INSTANCES";
    case Subsystem::PDM:         return "PDM";
    default: return QString("Unknown(%1)").arg(static_cast<int>(subsystem));
    }
}

inline QString genSubsystemTypeToString(GenSubsystemType type) {
    switch(type) {
    case GenSubsystemType::NOTGEN:       return "NOTGEN";
    case GenSubsystemType::PRODUCTION:   return "PRODUCTION";
    case GenSubsystemType::INTERGATION:  return "INTERGATION";
    default: return QString("Unknown(%1)").arg(static_cast<int>(type));
    }
}

// Операторы для QDebug
inline QDebug operator<<(QDebug debug, Subsystem subsystem) {
    QDebugStateSaver saver(debug);
    switch(subsystem) {
    case Subsystem::PROTOCOL:    debug.nospace() << "PROTOCOL"; break;
    case Subsystem::MATERIALS:   debug.nospace() << "MATERIALS"; break;
    case Subsystem::DESIGN:      debug.nospace() << "DESIGN"; break;
    case Subsystem::PURCHASES:   debug.nospace() << "PURCHASES"; break;
    case Subsystem::MANAGER:     debug.nospace() << "MANAGER"; break;
    case Subsystem::GENERATIVE:  debug.nospace() << "GENERATIVE"; break;
    case Subsystem::INSTANCES:   debug.nospace() << "INSTANCES"; break;
    case Subsystem::PDM:         debug.nospace() << "PDM"; break;
    default: debug.nospace() << "Unknown(" << static_cast<int>(subsystem) << ")";
    }
    return debug;
}

inline QDebug operator<<(QDebug debug, GenSubsystemType type) {
    QDebugStateSaver saver(debug);
    switch(type) {
    case GenSubsystemType::NOTGEN:       debug.nospace() << "NOTGEN"; break;
    case GenSubsystemType::PRODUCTION:   debug.nospace() << "PRODUCTION"; break;
    case GenSubsystemType::INTERGATION:  debug.nospace() << "INTERGATION"; break;
    default: debug.nospace() << "Unknown(" << static_cast<int>(type) << ")";
    }
    return debug;
}

inline QDebug operator<<(QDebug debug, ResourceType type) {
    QDebugStateSaver saver(debug);
    switch(type) {
    case ResourceType::DEFAULT:           debug.nospace() << "DEFAULT"; break;
    case ResourceType::EMPLOYEES:         debug.nospace() << "EMPLOYEES"; break;
    case ResourceType::PRODUCTION:        debug.nospace() << "PRODUCTION"; break;
    case ResourceType::INTEGRATION:       debug.nospace() << "INTEGRATION"; break;
    case ResourceType::GROUP:             debug.nospace() << "GROUP"; break;
    case ResourceType::PURCHASE:          debug.nospace() << "PURCHASE"; break;
    case ResourceType::PROJECT:           debug.nospace() << "PROJECT"; break;
    case ResourceType::ASSEMBLY:          debug.nospace() << "ASSEMBLY"; break;
    case ResourceType::PART:              debug.nospace() << "PART"; break;
    case ResourceType::SNAPSHOT:          debug.nospace() << "SNAPSHOT"; break;
    case ResourceType::DELTA:             debug.nospace() << "DELTA"; break;
    case ResourceType::MATERIAL_INSTANCE: debug.nospace() << "MATERIAL_INSTANCE"; break;
    default: debug.nospace() << "Unknown(" << static_cast<int>(type) << ")";
    }
    return debug;
}

inline QDebug operator<<(QDebug debug, CrudAction action) {
    QDebugStateSaver saver(debug);
    switch(action) {
    case CrudAction::NOTCRUD:  debug.nospace() << "NOTCRUD"; break;
    case CrudAction::CREATE:   debug.nospace() << "CREATE"; break;
    case CrudAction::READ:     debug.nospace() << "READ"; break;
    case CrudAction::UPDATE:   debug.nospace() << "UPDATE"; break;
    case CrudAction::DELETE:   debug.nospace() << "DELETE"; break;
    default: debug.nospace() << "Unknown(" << static_cast<int>(action) << ")";
    }
    return debug;
}

inline QDebug operator<<(QDebug debug, ProtocolAction action) {
    QDebugStateSaver saver(debug);
    switch(action) {
    case ProtocolAction::NOTPROTOCOL:             debug.nospace() << "NOTPROTOCOL"; break;
    case ProtocolAction::AUTH:                    debug.nospace() << "AUTH"; break;
    case ProtocolAction::FULL_SYNC:               debug.nospace() << "FULL_SYNC"; break;
    case ProtocolAction::GET_KAFKA_CREDENTIALS:   debug.nospace() << "GET_KAFKA_CREDENTIALS"; break;
    case ProtocolAction::GET_MINIO_PRESIGNED_URL: debug.nospace() << "GET_MINIO_PRESIGNED_URL"; break;
    case ProtocolAction::GET_MINIO_FILE:          debug.nospace() << "GET_MINIO_FILE"; break;
    case ProtocolAction::UPDATE_CHECK:            debug.nospace() << "UPDATE_CHECK"; break;
    case ProtocolAction::UPDATE_CONSENT:          debug.nospace() << "UPDATE_CONSENT"; break;
    case ProtocolAction::UPDATE_DOWNLOAD:         debug.nospace() << "UPDATE_DOWNLOAD"; break;
    default: debug.nospace() << "Unknown(" << static_cast<int>(action) << ")";
    }
    return debug;
}

inline QDebug operator<<(QDebug debug, MessageStatus status) {
    QDebugStateSaver saver(debug);
    switch(status) {
    case MessageStatus::REQUEST:       debug.nospace() << "REQUEST"; break;
    case MessageStatus::RESPONSE:      debug.nospace() << "RESPONSE"; break;
    case MessageStatus::ERROR:         debug.nospace() << "ERROR"; break;
    case MessageStatus::NOTIFICATION:  debug.nospace() << "NOTIFICATION"; break;
    case MessageStatus::SUCCESS:       debug.nospace() << "SUCCESS"; break;
    default: debug.nospace() << "Unknown(" << static_cast<int>(status) << ")";
    }
    return debug;
}

inline QDebug operator<<(QDebug debug, ErrorCode code) {
    QDebugStateSaver saver(debug);
    switch(code) {
    case ErrorCode::SUCCESS:              debug.nospace() << "SUCCESS(0)"; break;
    case ErrorCode::BAD_REQUEST:          debug.nospace() << "BAD_REQUEST(400)"; break;
    case ErrorCode::BAD_TIMING:           debug.nospace() << "BAD_TIMING(401)"; break;
    case ErrorCode::PERMISSION_DENIED:    debug.nospace() << "PERMISSION_DENIED(423)"; break;
    case ErrorCode::INTERNAL_ERROR:       debug.nospace() << "INTERNAL_ERROR(500)"; break;
    case ErrorCode::SERVICE_UNAVAILABLE:  debug.nospace() << "SERVICE_UNAVAILABLE(503)"; break;
    default: debug.nospace() << "Unknown(" << static_cast<int>(code) << ")";
    }
    return debug;
}

} // namespace uniter::contract

#endif // UNITERPROTOCOL_H
