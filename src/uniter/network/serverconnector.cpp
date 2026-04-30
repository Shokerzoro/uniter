
#include "serverconnector.h"
#include "../contract_qt/qt_compat.h"

#include "../contract/unitermessage.h"
#include "../contract/manager/employee.h"
#include "../contract/manager/permissions.h"

#include <QDebug>
#include <QDateTime>
#include <QTimer>

namespace {

// --------------------------------------------------------------------------
// Creating a mock user with ALL subsystems and full rights.
// Stub: in the user’s real system, a server is created based on the company’s database.
// --------------------------------------------------------------------------
std::shared_ptr<uniter::contract::manager::Employee> makeFullAccessUser()
{
    namespace C = uniter::contract;
    namespace E = uniter::contract::manager;
    using C::Subsystem;
    using C::GenSubsystem;
    using E::EmployeeAssignment;
    using E::Employee;

    const auto now_tp = std::chrono::system_clock::now();

    auto user = std::make_shared<Employee>(
        1,
        true,
        now_tp,
        now_tp,
        0,
        0,
        std::string("Test"),
        std::string("test.user@example.com"),
        std::string("..."),                 // or "", or hash, as in the database
        std::string("Test"),
        std::string("User"),
        std::nullopt,                       // phone can be NULL in the database
        std::vector<EmployeeAssignment>{}
    );

    // A set of rights for each static subsystem (enum uint8_t values).
    // We take the maximum value from uniter::contract::manager::*Permission —
    // fill in ALL values. This guarantees "full rights".
    auto makePerms = [](uint8_t count) {
        std::vector<uint8_t> v;
        v.reserve(count);
        for (uint8_t i = 0; i < count; ++i) v.push_back(i);
        return v;
    };

    auto makeAssign = [](Subsystem s, std::vector<uint8_t> perms) {
        EmployeeAssignment a;
        a.subsystem     = s;
        a.genSubsystem  = GenSubsystem::NOTGEN;
        a.genId         = std::nullopt;
        a.permissions   = std::move(perms);
        return a;
    };

    std::vector<EmployeeAssignment> assignments;
    assignments.push_back(makeAssign(Subsystem::MANAGER,   makePerms(10)));
    assignments.push_back(makeAssign(Subsystem::MATERIALS, makePerms(3)));
    assignments.push_back(makeAssign(Subsystem::INSTANCES, makePerms(3)));
    assignments.push_back(makeAssign(Subsystem::DESIGN,    makePerms(5)));
    assignments.push_back(makeAssign(Subsystem::PURCHASES, makePerms(4)));
    // PDM: there is no dedicated enum - we give a reasonable set of “all flags”.
    assignments.push_back(makeAssign(Subsystem::PDM,       makePerms(5)));
    assignments.push_back(makeAssign(Subsystem::DOCUMENTS, makePerms(5)));

    user->assignments = std::move(assignments);

    qDebug() << "ServerConnector: created stub user with ALL subsystems and full access";
    return user;
}

// --------------------------------------------------------------------------
// Generating a response to a protocol AUTH is always a success.
// --------------------------------------------------------------------------
std::shared_ptr<uniter::contract::UniterMessage>
makeProtocolAuthResponse(const std::shared_ptr<uniter::contract::UniterMessage>& request)
{
    using namespace uniter::contract;

    auto response = std::make_shared<UniterMessage>(*request);
    response->status    = MessageStatus::RESPONSE;
    response->protact   = ProtocolAction::AUTH;
    response->subsystem = Subsystem::PROTOCOL;
    response->error     = ErrorCode::SUCCESS;
    response->resource  = makeFullAccessUser();
    return response;
}

// --------------------------------------------------------------------------
// CRUD response: always positive. RESPONSE/SUCCESS, preserving the body
// original request for the client.
// --------------------------------------------------------------------------
std::shared_ptr<uniter::contract::UniterMessage>
makeCrudResponse(const std::shared_ptr<uniter::contract::UniterMessage>& request)
{
    using namespace uniter::contract;

    auto response = std::make_shared<UniterMessage>(*request);
    response->status = MessageStatus::RESPONSE;
    response->error  = ErrorCode::SUCCESS;
    return response;
}

// --------------------------------------------------------------------------
// Imitation of broadcast via Kafka: the same CUD, but NOTIFICATION status.
// --------------------------------------------------------------------------
std::shared_ptr<uniter::contract::UniterMessage>
makeCrudNotification(const std::shared_ptr<uniter::contract::UniterMessage>& request)
{
    using namespace uniter::contract;

    auto notification = std::make_shared<UniterMessage>(*request);
    notification->status = MessageStatus::NOTIFICATION;
    notification->error  = ErrorCode::SUCCESS;
    return notification;
}

// --------------------------------------------------------------------------
// Answer to GET_KAFKA_CREDENTIALS: we always confirm the relevance of offset.
// --------------------------------------------------------------------------
std::shared_ptr<uniter::contract::UniterMessage>
makeKafkaOffsetCheckResponse(const std::shared_ptr<uniter::contract::UniterMessage>& request)
{
    using namespace uniter::contract;

    auto response = std::make_shared<UniterMessage>(*request);
    response->status    = MessageStatus::RESPONSE;
    response->protact   = ProtocolAction::GET_KAFKA_CREDENTIALS;
    response->subsystem = Subsystem::PROTOCOL;
    response->error     = ErrorCode::SUCCESS;

    // Echo offset for transparency in logs.
    std::string offset_echo;
    auto it = request->add_data.find("offset");
    if (it != request->add_data.end()) {
        offset_echo = it->second;
    }

    response->add_data.clear();
    response->add_data.emplace("offset", offset_echo);
    response->add_data.emplace("offset_actual", "true");

    qDebug() << "ServerConnector: offset check ACTUAL for offset=" << offset_echo;
    return response;
}

// --------------------------------------------------------------------------
// Answer to FULL_SYNC: instant SUCCESS without data streaming.
// --------------------------------------------------------------------------
std::shared_ptr<uniter::contract::UniterMessage>
makeFullSyncDoneResponse(const std::shared_ptr<uniter::contract::UniterMessage>& request)
{
    using namespace uniter::contract;

    auto response = std::make_shared<UniterMessage>(*request);
    response->status    = MessageStatus::SUCCESS;
    response->protact   = ProtocolAction::FULL_SYNC;
    response->subsystem = Subsystem::PROTOCOL;
    response->error     = ErrorCode::SUCCESS;
    response->add_data.clear();
    response->add_data.emplace("offset", "full-sync-offset");

    qDebug() << "ServerConnector: FULL_SYNC done (stub, no CRUD stream)";
    return response;
}

} // anonymous namespace

namespace uniter::net {

using namespace uniter::contract;

ServerConnector* ServerConnector::instance()
{
    static ServerConnector instance;
    return &instance;
}

ServerConnector::ServerConnector()
    : QObject(nullptr)
{
}

ServerConnector::~ServerConnector() = default;

// === Slots ===

void ServerConnector::onMakeConnection()
{
    // Stub: connection is always instant and successful.
    connected_ = true;
    qDebug() << "ServerConnector::onMakeConnection() — stub connected";
    // Through an event loop, so as not to pull FSM from the same stack.
    QTimer::singleShot(0, this, [this]() {
        emit signalConnectionUpdated(true);
    });
}

void ServerConnector::onSendMessage(std::shared_ptr<contract::UniterMessage> message)
{
    if (!connected_) {
        qDebug() << "ServerConnector::onSendMessage() — ignored, not connected";
        return;
    }
    if (!message) {
        return;
    }

    ++seq_id_sent_;

    // --- PROTOCOL: AUTH ---
    if (message->subsystem == Subsystem::PROTOCOL &&
        message->protact   == ProtocolAction::AUTH)
    {
        auto response = makeProtocolAuthResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);
        return;
    }

    // --- PROTOCOL: checking if Kafka offset is up to date ---
    if (message->subsystem == Subsystem::PROTOCOL &&
        message->protact   == ProtocolAction::GET_KAFKA_CREDENTIALS)
    {
        auto response = makeKafkaOffsetCheckResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);
        return;
    }

    // --- PROTOCOL: full synchronization request ---
    if (message->subsystem == Subsystem::PROTOCOL &&
        message->protact   == ProtocolAction::FULL_SYNC &&
        message->status    == MessageStatus::REQUEST)
    {
        auto response = makeFullSyncDoneResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);
        return;
    }

    // --- PROTOCOL: request presigned URL from the server ---
    // We pass the MinIOConnector through a temporary direct connect.
    // MinIOConnector will respond via onMinioPresignedUrlReady, below.
    if (message->subsystem == Subsystem::PROTOCOL &&
        message->protact   == ProtocolAction::GET_MINIO_PRESIGNED_URL &&
        message->status    == MessageStatus::REQUEST)
    {
        QString object_key;
        QString minio_operation = QStringLiteral("GET");
        auto itk = message->add_data.find("object_key");
        if (itk != message->add_data.end()) object_key = QString::fromStdString(itk->second);
        auto ito = message->add_data.find("minio_operation");
        if (ito != message->add_data.end()) minio_operation = QString::fromStdString(ito->second);

        qDebug() << "ServerConnector: forwarding MinIO presigned URL request, key="
                 << object_key << " op=" << minio_operation;

        // Save a copy of the request to save the correlation
        // (request_sequence_id, add_data, etc.).
        auto requestCopy = std::make_shared<UniterMessage>(*message);
        emit signalRequestMinioPresignedUrl(requestCopy, object_key, minio_operation);
        return;
    }

    // --- CRUD: respond RESPONSE/SUCCESS and send NOTIFICATION via KafkaConnector ---
    if (message->crudact != CrudAction::NOTCRUD) {
        auto response = makeCrudResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);

        // TEMP broadcast: simulate Kafka notification about this CUD operation.
        // On a real Kafka server, the Kafka message is published by the server itself, without the participation of the client.
        auto notification = makeCrudNotification(message);
        emit signalEmitKafkaNotification(notification);
        return;
    }

    qDebug() << "ServerConnector::onSendMessage() — unhandled message"
             << "subsystem=" << message->subsystem
             << "protact="   << message->protact
             << "crudact="   << message->crudact
             << "status="    << message->status;
}

void ServerConnector::onMinioPresignedUrlReady(std::shared_ptr<contract::UniterMessage> requestCopy,
                                               QString object_key,
                                               QString presigned_url)
{
    // We form a RESPONSE for the initial request GET_MINIO_PRESIGNED_URL.
    auto response = std::make_shared<UniterMessage>();
    if (requestCopy) {
        *response = *requestCopy;
    }
    response->subsystem = Subsystem::PROTOCOL;
    response->protact   = ProtocolAction::GET_MINIO_PRESIGNED_URL;
    response->status    = MessageStatus::RESPONSE;
    response->error     = ErrorCode::SUCCESS;
    response->add_data.clear();
    response->add_data.emplace("object_key",    object_key.toStdString());
    response->add_data.emplace("presigned_url", presigned_url.toStdString());
    // url_expires_at is omitted - the stub URL is “indefinite”.

    qDebug() << "ServerConnector: returning presigned URL for key=" << object_key;
    ++seq_id_received_;
    emit signalRecvMessage(response);
}

void ServerConnector::onShutdown()
{
    if (connected_) {
        connected_ = false;
        emit signalConnectionUpdated(false);
    }
}

} // namespace uniter::net
