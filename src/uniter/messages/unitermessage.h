
#ifndef UNITERMESSAGE_H
#define UNITERMESSAGE_H

#include "../resources/resourceabstract.h"
#include "tinyxml2.h"
#include <QString>
#include <optional>
#include <memory>

namespace uniter::messages {


enum class Subsystem : uint8_t {
    PROTOCOL               = 0,
    MATERIALS              = 1, // Подсистема материалов
    DESIGN                 = 2, // конструктоская
    PURCHASES              = 3, // Подсистема закупок
    MANAGER                = 4,
    GENERATIVE             = 5
};

// Для генеративных подсистем
enum class GenSubsystemType : uint8_t {
    NOTGEN                 = 0,
    PRODUCTION             = 1,
    INTERGATION            = 2,
};

enum class CrudAction : uint8_t {
    NOTCRUD                = 0,
    CREATE                 = 1,
    READ                   = 2,
    UPDATE                 = 3,
    DELETE                 = 4
};

enum class ProtocolAction : uint8_t {
    NOTPROTOCOL            = 0,
    ALOCATE_ID             = 1, // Получить id ресурса (sequence_id в случае PROTOCOL)
    GETCONFIG              = 2, // Запрос конфигурации
    POLL                   = 3, // Для запроса сообщений
    SYNC                   = 4  // Синхронизация новичка
};

enum class ResourceType : uint8_t {
    PROTOCOL               = 0,
    // Менеджер
    EMPLOYEE               = 1,
    PRODUCTION             = 2,
    INTEGRATION            = 3,
    // Закупки
    GROUP                  = 4,
    PURCHASE               = 5,
    // Проектирование
    PROJECT                = 6,
    ASSEMBLY               = 7,
    PART                   = 8,
    // Производство
    PRUDUCTION_TASK        = 9,
    // Интеграция
    INTEGRATION_TASK       = 10
};

enum class MessageStatus : uint8_t {
    REQUEST                = 0,
    RESPONSE               = 1,
    NOTIFICATION           = 2
};

enum class ErrorCode : uint16_t {
    SUCCESS                = 1,
    BAD_REQUEST            = 400,
    BAD_TIMING             = 401, // Устаревший id, сетевой класс переотправляет
    PERMISSION_DENIED      = 423, // Устаревшая конфигурация
    INTERNAL_ERROR         = 500,
};


class UniterMessage {
public:
    UniterMessage() = default;
    // Метаданные
    uint32_t version;
    QDateTime timestamp;
    std::optional<uint64_t> message_sequence_id;
    std::optional<uint64_t> request_sequence_id;

    // Определение подсистемы
    Subsystem subsystem;
    GenSubsystemType GenSubsystem = GenSubsystemType::NOTGEN;
    uint64_t GenSubsystemId = 0;

    // Определение назначения и жизненного цикла
    CrudAction crudact = CrudAction::NOTCRUD;
    ProtocolAction protact = ProtocolAction::NOTPROTOCOL;
    MessageStatus status = MessageStatus::REQUEST;
    ErrorCode error = ErrorCode::SUCCESS;

    // Ресурсы и данные
    std::shared_ptr<resources::ResourceAbstract> resource;
    std::map<QString, QString> add_data;

    // Сериализация десериализция
    void to_xml(tinyxml2::XMLElement* xmlel) const;
    void form_xml(tinyxml2::XMLElement* xmlel);
};


inline QString subsystemToString(Subsystem subsystem) {
    switch (subsystem) {
    case Subsystem::DESIGN: return "DESIGN";
    case Subsystem::GENERATIVE: return "GENERATIVE";
    case Subsystem::MANAGER: return "MANAGER";
    case Subsystem::MATERIALS: return "MATERIALS";
    case Subsystem::PROTOCOL: return "PROTOCOL";
    case Subsystem::PURCHASES: return "PURCHASES";
    default: return "UNKNOWN";
    }
}

// Опционально: перегрузка оператора << для прямого логирования
inline QDebug operator<<(QDebug debug, Subsystem subsystem) {
    debug.noquote() << subsystemToString(subsystem);
    return debug;
}


} // messages


#endif //UNITERMESSAGE_H
