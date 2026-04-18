
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

    // Поля для операций с MinIO (Subsystem::PROTOCOL + GET_MINIO_PRESIGNED_URL / GET_MINIO_FILE)
    // minio_object_key  — ключ объекта в MinIO (путь внутри bucket-а)
    // minio_presigned_url — временный presigned URL, полученный от основного сервера
    std::optional<QString> minio_object_key;
    std::optional<QString> minio_presigned_url;

    // Сериализация десериализция
    void to_xml(tinyxml2::XMLElement* xmlel) const;
    void form_xml(tinyxml2::XMLElement* xmlel);
};



} // uniter::contract


#endif //UNITERMESSAGE_H
