
#ifndef UNITERMESSAGE_H
#define UNITERMESSAGE_H

#include "uniterprotocol.h"
#include "resourceabstract.h"
#include "tinyxml2.h"
#include <QString>
#include <optional>
#include <memory>


namespace uniter::contract {


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
    std::shared_ptr<contract::ResourceAbstract> resource;
    std::map<QString, QString> add_data;

    // Сериализация десериализция
    void to_xml(tinyxml2::XMLElement* xmlel) const;
    void form_xml(tinyxml2::XMLElement* xmlel);
};



} // uniter::contract


#endif //UNITERMESSAGE_H
