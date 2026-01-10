
#ifndef UNITERMESSAGE_H
#define UNITERMESSAGE_H

#include "../resources/resourceabstract.h"
#include "tinyxml2.h"
#include <optional>

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
    PRODUCTION             = 0,
    INTERGATION            = 1,
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
    INTEGRATION            = 10
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
    explicit UniterMessage();
    // Метаданные
    uint32_t version;
    std::optional<uint64_t> message_sequence_id;
    std::optional<uint64_t> request_sequence_id;
    QDateTime timestamp;

    // Определение подсистемы
    Subsystem subsystem;
    std::optional<GenSubsystemType> GenSubsystem;
    std::optional<uint64_t> GenSubsystemId;

    // Определение назначения и жизненного цикла
    CrudAction crudact;
    ProtocolAction protact;
    MessageStatus status;
    ErrorCode error;

    // Ресурсы (только один тип активен)
    std::optional<ResourceType> resource_type; // Также для передачи Emploee для конфигурации
    std::optional<std::vector<resources::ResourceAbstract>> resource_instance;
    // Данные (для передачи sequnce_id, resource_id, авторизации)
    std::map<QString, QString> add_data;

    // Сериализация десериализция
    void to_xml(tinyxml2::XMLElement* xmlel) const;
    void form_xml(tinyxml2::XMLElement* xmlel);
};


} // messages


#endif //UNITERMESSAGE_H
