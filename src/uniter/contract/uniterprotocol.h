#ifndef UNITERPROTOCOL_H
#define UNITERPROTOCOL_H

#include <cstdint>
#include <QDebug>

namespace uniter::contract {

enum class Subsystem : uint8_t {
    PROTOCOL                = 0,
    MATERIALS               = 1,
    DESIGN                  = 2,
    PURCHASES               = 3,
    MANAGER                 = 4,
    GENERATIVE              = 5,
    INSTANCES               = 6,
    PDM                     = 7,
    DOCUMENTS               = 8
};

enum class GenSubsystemType : uint8_t {
    NOTGEN                  = 0,
    PRODUCTION              = 1,
    INTERGATION             = 2,
};

enum class ResourceType : uint8_t {
    DEFAULT                  = 0,

    // --- Subsystem::MANAGER ---
    EMPLOYEES                = 10,
    PRODUCTION               = 11,   // Генеративный: создание порождает GenSubsystem::PRODUCTION
    INTEGRATION              = 12,   // Генеративный: создание порождает GenSubsystem::INTERGATION
    EMPLOYEE_ASSIGNMENT      = 13,   // manager/employee_assignment (свёрнуто в Employee.assignments)
    PERMISSION               = 14,   // manager/permissions (1:M к employee_assignment)
    EMPLOYEE_ASSIGNMENT_LINK = 15,   // manager/employee_assignment_link (junction employee↔assignment, M:N)

    // --- Subsystem::MATERIALS ---
    MATERIAL_TEMPLATE_SIMPLE    = 20,
    MATERIAL_TEMPLATE_COMPOSITE = 21,
    SEGMENT                     = 22,   // material/segment (свёрнуто в TemplateSimple.prefix_segments / suffix_segments)
    SEGMENT_VALUE               = 23,   // material/segment_value (1:M к segment; в SegmentDefinition.allowed_values)
    TEMPLATE_COMPATIBILITY      = 24,   // material/template_compatibility (M:N template_simple ↔ template_simple)

    // --- Subsystem::DESIGN ---
    PROJECT                  = 30,   // design_project
    ASSEMBLY                 = 31,   // design_assembly (общие данные сборки)
    PART                     = 32,   // design_part     (общие данные детали)
    ASSEMBLY_CONFIG          = 33,   // design_assembly_config + join-таблицы
    PART_CONFIG              = 34,   // design_part_config (исполнение детали)

    // --- Subsystem::PURCHASES ---
    PURCHASE_GROUP           = 40,   // Комплексная заявка
    PURCHASE                 = 41,   // ProcurementRequest

    // --- Subsystem::PDM ---
    SNAPSHOT                 = 50,   // pdm_snapshot
    DELTA                    = 51,   // pdm_delta
    PDM_PROJECT              = 52,   // pdm_project (версионная ветка)
    // PDM-зеркала классов DESIGN (см. docs/db/pdm.md). Классы переиспользуются
    // из namespace design::*, отличие — ResourceType и таблица-приёмник в БД.
    ASSEMBLY_PDM             = 53,   // pdm_assembly
    ASSEMBLY_CONFIG_PDM      = 54,   // pdm_assembly_config + join-таблицы
    PART_PDM                 = 55,   // pdm_part
    PART_CONFIG_PDM          = 56,   // pdm_part_config
    DIAGNOSTIC               = 57,   // pdm_diagnostic (ошибки/замечания парсинга, N:M к snapshot)

    // --- Subsystem::INSTANCES ---
    // По ЕСКД Instance бывает двух структурных вариантов, хранятся в
    // разных таблицах (material_instances_simple / material_instances_composite).
    // Прежний обобщённый MATERIAL_INSTANCE = 60 удалён.
    MATERIAL_INSTANCE_SIMPLE    = 61,   // material_instances_simple
    MATERIAL_INSTANCE_COMPOSITE = 62,   // material_instances_composite

    // --- Subsystem::GENERATIVE + GenSubsystem::PRODUCTION ---
    PRODUCTION_TASK          = 70,   // production_task
    PRODUCTION_STOCK         = 71,   // Позиция склада: ссылка на MaterialInstance + количество
    PRODUCTION_SUPPLY        = 72,   // Связь ProductionTask ↔ PurchaseComplex

    // --- Subsystem::GENERATIVE + GenSubsystem::INTERGATION ---
    INTEGRATION_TASK         = 80,

    // --- Subsystem::DOCUMENTS ---
    DOC                      = 90,   // Внешний документ (файл в MinIO: чертёж/PDF ГОСТ/3D-модель)
    DOC_LINK                 = 91,   // «Папка» документов: 1:M к Doc (свёрнута в DocLink.docs); ссылка со стороны владельца через doc_link_id
};

enum class CrudAction : uint8_t {
    NOTCRUD                = 0,
    CREATE                 = 1,
    READ                   = 2,
    UPDATE                 = 3,
    DELETE                 = 4
};

// Все протокольные действия находятся в домене Subsystem::PROTOCOL.
// Полная спецификация ключей add_data для каждого действия: см. docs/add_data_convention.md
enum class ProtocolAction : uint8_t {
    NOTPROTOCOL            = 0,
    // Аутентификация.
    // REQUEST: add_data["login", "password_hash"]; RESPONSE: resource = Employee
    AUTH                   = 1,
    // Полная синхронизация: сервер присылает поток CRUD CREATE через Kafka.
    // REQUEST: нет параметров; SUCCESS: маркер завершения (нет ключей)
    FULL_SYNC              = 2,
    // Запрос credentials для подключения к топику Kafka.
    // RESPONSE: add_data["bootstrap_servers", "topic", "username", "password", "group_id"]
    GET_KAFKA_CREDENTIALS  = 3,
    // Запрос presigned URL у основного сервера для доступа к объекту MinIO.
    // REQUEST:  add_data["object_key", "minio_operation"("GET"|"PUT")]
    // RESPONSE: add_data["object_key", "presigned_url", "url_expires_at"]
    GET_MINIO_PRESIGNED_URL = 4,
    // Запрос MinIOConnector скачать файл по presigned URL.
    // REQUEST:  add_data["presigned_url", "object_key"]; опц.: add_data["expected_sha256"]
    // RESPONSE: add_data["object_key", "local_path"]
    // ERROR:    add_data["object_key", "reason"]
    GET_MINIO_FILE         = 5,
    // Запрос MinIOConnector загрузить файл в MinIO по presigned URL.
    // REQUEST:  add_data["presigned_url", "object_key", "local_path"]
    // RESPONSE: add_data["object_key", "presigned_url"]
    // ERROR:    add_data["object_key", "reason"]
    PUT_MINIO_FILE         = 9,
    // Проверка наличия новой версии приложения.
    // REQUEST: add_data["current_version"]; RESPONSE: add_data["update_available", "new_version", "release_notes"]
    UPDATE_CHECK           = 6,
    // Согласие/отказ пользователя на установку. Чисто клиентское действие — на сервер не отправляется.
    // add_data["accepted"] = "true"|"false"; UpdaterWorker при accepted=true инициирует UPDATE_DOWNLOAD
    UPDATE_CONSENT         = 7,
    // Получение файлов обновления.
    // REQUEST:      add_data["version"]
    // NOTIFICATION: каждый файл отдельным сообщением;
    //              add_data["file_path", "presigned_url", "sha256", "file_index", "files_total", "file_size"]
    // SUCCESS:      все файлы отправлены; add_data["version", "files_total"]
    UPDATE_DOWNLOAD        = 8,
};

enum class MessageStatus : uint8_t {
    REQUEST                = 0,
    RESPONSE               = 1,
    ERROR                  = 2,
    NOTIFICATION           = 3,
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
    case Subsystem::DOCUMENTS:   return "DOCUMENTS";
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
    case Subsystem::DOCUMENTS:   debug.nospace() << "DOCUMENTS"; break;
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
    case ResourceType::DEFAULT:                      debug.nospace() << "DEFAULT"; break;
    case ResourceType::EMPLOYEES:                    debug.nospace() << "EMPLOYEES"; break;
    case ResourceType::PRODUCTION:                   debug.nospace() << "PRODUCTION"; break;
    case ResourceType::INTEGRATION:                  debug.nospace() << "INTEGRATION"; break;
    case ResourceType::EMPLOYEE_ASSIGNMENT:          debug.nospace() << "EMPLOYEE_ASSIGNMENT"; break;
    case ResourceType::PERMISSION:                   debug.nospace() << "PERMISSION"; break;
    case ResourceType::EMPLOYEE_ASSIGNMENT_LINK:     debug.nospace() << "EMPLOYEE_ASSIGNMENT_LINK"; break;
    case ResourceType::MATERIAL_TEMPLATE_SIMPLE:     debug.nospace() << "MATERIAL_TEMPLATE_SIMPLE"; break;
    case ResourceType::MATERIAL_TEMPLATE_COMPOSITE:  debug.nospace() << "MATERIAL_TEMPLATE_COMPOSITE"; break;
    case ResourceType::SEGMENT:                      debug.nospace() << "SEGMENT"; break;
    case ResourceType::SEGMENT_VALUE:                debug.nospace() << "SEGMENT_VALUE"; break;
    case ResourceType::TEMPLATE_COMPATIBILITY:       debug.nospace() << "TEMPLATE_COMPATIBILITY"; break;
    case ResourceType::PROJECT:                      debug.nospace() << "PROJECT"; break;
    case ResourceType::ASSEMBLY:                     debug.nospace() << "ASSEMBLY"; break;
    case ResourceType::PART:                         debug.nospace() << "PART"; break;
    case ResourceType::ASSEMBLY_CONFIG:              debug.nospace() << "ASSEMBLY_CONFIG"; break;
    case ResourceType::PART_CONFIG:                  debug.nospace() << "PART_CONFIG"; break;
    case ResourceType::ASSEMBLY_PDM:                 debug.nospace() << "ASSEMBLY_PDM"; break;
    case ResourceType::ASSEMBLY_CONFIG_PDM:          debug.nospace() << "ASSEMBLY_CONFIG_PDM"; break;
    case ResourceType::PART_PDM:                     debug.nospace() << "PART_PDM"; break;
    case ResourceType::PART_CONFIG_PDM:              debug.nospace() << "PART_CONFIG_PDM"; break;
    case ResourceType::PURCHASE_GROUP:               debug.nospace() << "PURCHASE_GROUP"; break;
    case ResourceType::PURCHASE:                     debug.nospace() << "PURCHASE"; break;
    case ResourceType::SNAPSHOT:                     debug.nospace() << "SNAPSHOT"; break;
    case ResourceType::DELTA:                        debug.nospace() << "DELTA"; break;
    case ResourceType::PDM_PROJECT:                  debug.nospace() << "PDM_PROJECT"; break;
    case ResourceType::DIAGNOSTIC:                   debug.nospace() << "DIAGNOSTIC"; break;
    case ResourceType::MATERIAL_INSTANCE_SIMPLE:     debug.nospace() << "MATERIAL_INSTANCE_SIMPLE"; break;
    case ResourceType::MATERIAL_INSTANCE_COMPOSITE:  debug.nospace() << "MATERIAL_INSTANCE_COMPOSITE"; break;
    case ResourceType::PRODUCTION_TASK:              debug.nospace() << "PRODUCTION_TASK"; break;
    case ResourceType::PRODUCTION_STOCK:             debug.nospace() << "PRODUCTION_STOCK"; break;
    case ResourceType::PRODUCTION_SUPPLY:            debug.nospace() << "PRODUCTION_SUPPLY"; break;
    case ResourceType::INTEGRATION_TASK:             debug.nospace() << "INTEGRATION_TASK"; break;
    case ResourceType::DOC:                          debug.nospace() << "DOC"; break;
    case ResourceType::DOC_LINK:                     debug.nospace() << "DOC_LINK"; break;
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
    case ProtocolAction::PUT_MINIO_FILE:          debug.nospace() << "PUT_MINIO_FILE"; break;
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
