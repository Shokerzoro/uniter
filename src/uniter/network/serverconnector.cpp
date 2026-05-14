
#include "serverconnector.h"

#include "../contract/unitermessage.h"
#include "../contract/manager/employee.h"
#include "../contract/manager/permissions.h"

#include <QDebug>
#include <QDateTime>
#include <QTimer>

namespace {

// --------------------------------------------------------------------------
// Создание мок-пользователя со ВСЕМИ подсистемами и полными правами.
// Stub: в реальной системе пользователя формирует сервер по БД компании.
// --------------------------------------------------------------------------
std::shared_ptr<uniter::contract::employees::Employee> makeFullAccessUser()
{
    namespace C = uniter::contract;
    namespace E = uniter::contract::employees;
    using C::Subsystem;
    using E::EmployeeAssignment;
    using E::Employee;

    auto user = std::make_shared<Employee>(
        /*sid*/ 1,
        /*actual*/ true,
        QDateTime::currentDateTime(),
        QDateTime::currentDateTime(),
        /*created_by*/ 0,
        /*updated_by*/ 0,
        QStringLiteral("Test"),
        QStringLiteral("User"),
        QStringLiteral("Stub"),
        QStringLiteral("test.user@example.com"),
        std::vector<EmployeeAssignment>{}
    );

    // Набор прав по каждой статической подсистеме (enum uint8_t значения).
    // Берём по максимальному значению из uniter::contract::employees::*Permission —
    // заполняем ВСЕ значения. Это гарантирует «полные права».
    auto makePerms = [](uint8_t count) {
        std::vector<uint8_t> v;
        v.reserve(count);
        for (uint8_t i = 0; i < count; ++i) v.push_back(i);
        return v;
    };

    auto makeAssign = [](Subsystem s, std::vector<uint8_t> perms) {
        EmployeeAssignment a;
        a.subsystem     = s;
        a.subsystemInstanceId = std::nullopt;
        a.permissions   = std::move(perms);
        return a;
    };

    std::vector<EmployeeAssignment> assignments;
    assignments.push_back(makeAssign(Subsystem::MANAGER,   makePerms(10)));
    assignments.push_back(makeAssign(Subsystem::MATERIALS, makePerms(3)));
    assignments.push_back(makeAssign(Subsystem::INSTANCES, makePerms(3)));
    assignments.push_back(makeAssign(Subsystem::DESIGN,    makePerms(5)));
    assignments.push_back(makeAssign(Subsystem::PURCHASES, makePerms(4)));
    // PDM: выделенного enum нет — даём разумный набор «всех флагов».
    assignments.push_back(makeAssign(Subsystem::PDM,       makePerms(5)));
    assignments.push_back(makeAssign(Subsystem::DOCUMENTS, makePerms(5)));

    user->assignments = std::move(assignments);

    qDebug() << "ServerConnector: created stub user with ALL subsystems and full access";
    return user;
}

// --------------------------------------------------------------------------
// Формирование ответа на протокольный AUTH — всегда успех.
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
// CRUD-ответ: всегда положительный. RESPONSE/SUCCESS, сохраняя тело
// исходного запроса для клиента.
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
// Имитация broadcast через Kafka: тот же CUD, но статус NOTIFICATION.
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
// Ответ на GET_KAFKA_CREDENTIALS: всегда подтверждаем актуальность offset.
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

    // Echo offset для прозрачности в логах.
    QString offset_echo;
    auto it = request->add_data.find("offset");
    if (it != request->add_data.end()) {
        offset_echo = QString::fromStdString(it->second);
    }

    response->add_data.clear();
    response->add_data.emplace("offset", offset_echo.toStdString());
    response->add_data.emplace("offset_actual", "true");

    qDebug() << "ServerConnector: offset check ACTUAL for offset=" << offset_echo;
    return response;
}

// --------------------------------------------------------------------------
// Ответ на FULL_SYNC: мгновенное SUCCESS без стрима данных.
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

// === Слоты ===

void ServerConnector::onMakeConnection()
{
    // Stub: подключение всегда мгновенное и успешное.
    connected_ = true;
    qDebug() << "ServerConnector::onMakeConnection() — stub connected";
    // Через event loop, чтобы не дёргать FSM из того же стека.
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

    // --- PROTOCOL: проверка актуальности offset Kafka ---
    if (message->subsystem == Subsystem::PROTOCOL &&
        message->protact   == ProtocolAction::GET_KAFKA_CREDENTIALS)
    {
        auto response = makeKafkaOffsetCheckResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);
        return;
    }

    // --- PROTOCOL: запрос полной синхронизации ---
    if (message->subsystem == Subsystem::PROTOCOL &&
        message->protact   == ProtocolAction::FULL_SYNC &&
        message->status    == MessageStatus::REQUEST)
    {
        auto response = makeFullSyncDoneResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);
        return;
    }

    // --- PROTOCOL: запрос presigned URL у сервера ---
    // Перекидываем MinIOConnector-у через временный прямой connect.
    // MinIOConnector ответит через onMinioPresignedUrlReady, ниже.
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

        // Сохраняем копию запроса, чтобы сохранить correlation
        // (request_sequence_id, add_data и т.п.).
        auto requestCopy = std::make_shared<UniterMessage>(*message);
        emit signalRequestMinioPresignedUrl(requestCopy, object_key, minio_operation);
        return;
    }

    // --- CRUD: отвечаем RESPONSE/SUCCESS и шлём NOTIFICATION через KafkaConnector ---
    if (message->crudact != CrudAction::NOTCRUD) {
        auto response = makeCrudResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);

        // TEMP broadcast: имитируем Kafka-уведомление об этой CUD операции.
        // На реальном сервере Kafka-сообщение публикует сам сервер, без участия клиента.
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
    // Формируем RESPONSE для исходного запроса GET_MINIO_PRESIGNED_URL.
    auto response = std::make_shared<UniterMessage>();
    if (requestCopy) {
        *response = *requestCopy;
    }
    response->subsystem = Subsystem::PROTOCOL;
    response->protact   = ProtocolAction::GET_MINIO_PRESIGNED_URL;
    response->status    = MessageStatus::RESPONSE;
    response->error     = ErrorCode::SUCCESS;
    response->add_data.clear();
    response->add_data.emplace("object_key",   object_key.toStdString());
    response->add_data.emplace("presigned_url", presigned_url.toStdString());
    // url_expires_at опускаем — в stub URL «бессрочный».

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
