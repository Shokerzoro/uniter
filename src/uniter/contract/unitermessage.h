
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

    // Произвольные данные в виде key-value пар.
    // Используется для передачи параметров протокольных операций, например:
    //   GET_MINIO_PRESIGNED_URL (request):  add_data["object_key"] = "<MinIO object key>"
    //   GET_MINIO_PRESIGNED_URL (response): add_data["presigned_url"] = "<временный URL>"
    //   GET_MINIO_FILE (request):           add_data["presigned_url"] = "<URL для скачивания>"
    //   GET_KAFKA_CREDENTIALS (response):   add_data["bootstrap_servers"], add_data["topic"], add_data["username"], add_data["password"]
    //   UPDATE_CONSENT:                     add_data["accepted"] = "true" | "false"
    std::map<QString, QString> add_data;

    // Сериализация / десериализация
    void to_xml(tinyxml2::XMLElement* xmlel) const;
    void form_xml(tinyxml2::XMLElement* xmlel);
};



} // uniter::contract


#endif //UNITERMESSAGE_H
