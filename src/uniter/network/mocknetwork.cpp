
#include "../resources/employee/employee.h"
#include "mocknetwork.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>
#include <QTimer>
#include <future>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace {


// Создание мок-пользователя с рандомными подсистемами
std::shared_ptr<uniter::resources::employees::Employee> makeMockUserWithRandomSubsystems()
{
    auto user = std::make_shared<uniter::resources::employees::Employee>(
        /*sid*/ 1,
        /*actual*/ true,
        QDateTime::currentDateTime(),
        QDateTime::currentDateTime(),
        /*created_by*/ 0,
        /*updated_by*/ 0,
        QStringLiteral("Test"),
        QStringLiteral("User"),
        QStringLiteral("Mock"),
        QStringLiteral("test.user@example.com"),
        std::vector<uniter::resources::employees::EmployeeAssignment>{}
        );

    std::vector<uniter::resources::employees::EmployeeAssignment> assignments;
    QStringList subsystemNames;

    if (QRandomGenerator::global()->bounded(2) == 1) {
        assignments.push_back(uniter::resources::employees::EmployeeAssignment{
            uniter::messages::Subsystem::MANAGER,
            uniter::messages::GenSubsystemType::NOTGEN,
            std::nullopt,
            {}
        });
        subsystemNames << "MANAGER";
    }

    if (QRandomGenerator::global()->bounded(2) == 1) {
        assignments.push_back(uniter::resources::employees::EmployeeAssignment{
            uniter::messages::Subsystem::MATERIALS,
            uniter::messages::GenSubsystemType::NOTGEN,
            std::nullopt,
            {}
        });
        subsystemNames << "MATERIALS";
    }

    if (QRandomGenerator::global()->bounded(2) == 1) {
        assignments.push_back(uniter::resources::employees::EmployeeAssignment{
            uniter::messages::Subsystem::PURCHASES,
            uniter::messages::GenSubsystemType::NOTGEN,
            std::nullopt,
            {}
        });
        subsystemNames << "PURCHASES";
    }

    // Гарантируем хотя бы одну подсистему
    if (assignments.empty()) {
        assignments.push_back(uniter::resources::employees::EmployeeAssignment{
            uniter::messages::Subsystem::MANAGER,
            uniter::messages::GenSubsystemType::NOTGEN,
            std::nullopt,
            {}
        });
        subsystemNames << "MANAGER";
    }

    user->assignments = std::move(assignments);

    qDebug() << "MockNetManager: Created user with subsystems:" << subsystemNames.join(", ");

    return user;
}

// Формирование ответа на протокольный GETCONFIG/AUTH
std::shared_ptr<uniter::messages::UniterMessage> makeProtocolAuthResponse(const std::shared_ptr<uniter::messages::UniterMessage>& request)
{
    using namespace uniter::messages;

    auto response = std::make_shared<UniterMessage>(*request);
    response->status    = MessageStatus::RESPONSE;
    response->protact   = ProtocolAction::GETCONFIG;
    response->subsystem = Subsystem::PROTOCOL;

    bool success = QRandomGenerator::global()->bounded(100) < 70; // 70% успех

    if (success) {
        response->error    = ErrorCode::SUCCESS;
        response->resource = makeMockUserWithRandomSubsystems();
    } else {
        qDebug() << "MockNetManager: Auth FAILED";
        response->error = ErrorCode::BAD_REQUEST;
    }

    return response;
}

// Формирование CRUD-ответа (эхо с рандомным ErrorCode)
std::shared_ptr<uniter::messages::UniterMessage>
makeCrudResponse(const std::shared_ptr<uniter::messages::UniterMessage>& request)
{
    using namespace uniter::messages;

    auto response = std::make_shared<UniterMessage>(*request);
    response->status = MessageStatus::RESPONSE;

    bool ok = (QRandomGenerator::global()->bounded(2) == 1); // 50/50
    response->error = ok ? ErrorCode::SUCCESS : ErrorCode::BAD_REQUEST;

    return response;
}

} // anonymous namespace

namespace uniter::net {

using namespace uniter::messages;

MockNetManager::MockNetManager(QObject* parent)
    : QObject(parent)
{
}

MockNetManager::~MockNetManager() = default;

// === Слоты ===

void MockNetManager::onMakeConnection() {


    int roll = QRandomGenerator::global()->bounded(100);
    bool success = (roll < 30);

    connected_ = success;

    if (success) {
        qDebug() << "MockNetManager::onMakeConnection() - emit signalConnected()";
        emit signalConnected();

        // QTimer вместо std::async
        int seconds = QRandomGenerator::global()->bounded(60, 121);
        QTimer::singleShot(seconds * 1000, this, [this]() {
            if (connected_) {
                connected_ = false;
                qDebug() << "MockNetManager: auto-disconnect triggered";
                emit signalDisconnected();
            }
        });
    } else {
        qDebug() << "MockNetManager::onMakeConnection() - signalDisconnected()";
        emit signalDisconnected();
    }
}


void MockNetManager::onSendMessage(std::shared_ptr<messages::UniterMessage> message)
{
    if (!connected_) {
        return;
    }

    ++seq_id_sent_;

    // Протокол: PROTOCOL + GETCONFIG
    if (message->subsystem == Subsystem::PROTOCOL &&
        message->protact  == ProtocolAction::GETCONFIG)
    {
        auto response = makeProtocolAuthResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);
        return;
    }

    // CRUD: эхо с рандомным успехом/ошибкой
    if (message->crudact != CrudAction::NOTCRUD) {
        auto response = makeCrudResponse(message);
        ++seq_id_received_;
        emit signalRecvMessage(response);
        return;
    }

    // Остальное: тупое эхо
    auto echo = std::make_shared<UniterMessage>(*message);
    ++seq_id_received_;
    emit signalRecvMessage(echo);
}

void MockNetManager::onShutdown()
{
    if (connected_) {
        connected_ = false;
        emit signalDisconnected();
    }
}

} // namespace uniter::net
