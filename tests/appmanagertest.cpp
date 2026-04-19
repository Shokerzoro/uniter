#include "../src/uniter/control/appmanager.h"
#include "../src/uniter/control/configmanager.h"
#include "../src/uniter/contract/unitermessage.h"
#include "../src/uniter/contract/resourceabstract.h"
#include "../src/uniter/contract/employee/employee.h"
#include "../src/uniter/contract/supply/purchase.h"

#include <gtest/gtest.h>
#include <QObject>
#include <QSignalSpy>
#include <QCryptographicHash>
#include <QString>
#include <memory>

using namespace uniter;

namespace appmanagertest {

// ============================================================================
// Mock AuthWidget — имитирует пользовательский виджет входа.
// ============================================================================
class MockAuthWidget : public QObject {
    Q_OBJECT
public:
    MockAuthWidget() = default;
    bool authed = false;
    int  authedFalseCount = 0;

    void emitUserData() {
        auto authReqMessage = std::make_shared<contract::UniterMessage>();
        authReqMessage->subsystem = contract::Subsystem::PROTOCOL;
        authReqMessage->protact   = contract::ProtocolAction::AUTH;
        authReqMessage->status    = contract::MessageStatus::REQUEST;
        authReqMessage->add_data.emplace("login", "niger");
        authReqMessage->add_data.emplace("password", "12345");
        emit signalSendUniterMessage(authReqMessage);
    }
public slots:
    void onFindAuthData() {
        // В реальности — показать экран логина. Для теста ничего не делаем.
    }
    void onAuthed(bool result) {
        authed = result;
        if (!result) ++authedFalseCount;
    }
signals:
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
};

// ============================================================================
// Mock Network — имитирует и MockNetManager, и MinIOConnector.
// Это даёт тесту полный контроль над сценарием KAFKA/SYNC/READY.
// ============================================================================
class MockNetwork : public QObject {
    Q_OBJECT
public:
    MockNetwork() = default;

    int subscribeKafkaCount = 0;
    int initMinIOCount      = 0;
    int makeConnectionCount = 0;
    int pollMessagesCount   = 0;

    void emitConnectionUpdated(bool state) {
        emit signalConnectionUpdated(state);
    }

    // CRUD NOTIFICATION из "Kafka"
    void emitCrudMessage() {
        auto crudMessage = std::make_shared<contract::UniterMessage>();
        crudMessage->subsystem = contract::Subsystem::PURCHASES;
        crudMessage->crudact   = contract::CrudAction::CREATE;
        crudMessage->status    = contract::MessageStatus::NOTIFICATION;

        auto newPurch = std::make_shared<contract::supply::Purchase>(
            1001, true,
            QDateTime::currentDateTime(), QDateTime::currentDateTime(),
            12345, 12345,
            QString("Закупка стали 10т"),
            QString("Нужна сталь для проекта #47"),
            contract::supply::PurchStatus::DRAFT,
            std::nullopt);
        crudMessage->resource = std::move(newPurch);

        emit signalRecvUniterMessage(std::move(crudMessage));
    }

    // AUTH RESPONSE success (с Employee)
    void emitAuthSuccess() {
        auto authMessageSuccess = std::make_shared<contract::UniterMessage>();
        authMessageSuccess->subsystem = contract::Subsystem::PROTOCOL;
        authMessageSuccess->protact   = contract::ProtocolAction::AUTH;
        authMessageSuccess->status    = contract::MessageStatus::RESPONSE;
        authMessageSuccess->error     = contract::ErrorCode::SUCCESS;

        std::vector<contract::employees::EmployeeAssignment> assignments;
        auto employee = std::make_shared<contract::employees::Employee>(
            12345, true,
            QDateTime::currentDateTime(), QDateTime::currentDateTime(),
            0, 0,
            QString("Иван"), QString("Иванов"), QString("Иванович"),
            QString("ivan@company.ru"),
            std::move(assignments));
        authMessageSuccess->resource = std::move(employee);

        emit signalRecvUniterMessage(std::move(authMessageSuccess));
    }

    void emitAuthError() {
        auto errorMessage = std::make_shared<contract::UniterMessage>();
        errorMessage->version   = 1;
        errorMessage->subsystem = contract::Subsystem::PROTOCOL;
        errorMessage->protact   = contract::ProtocolAction::AUTH;
        errorMessage->status    = contract::MessageStatus::RESPONSE;
        errorMessage->error     = contract::ErrorCode::BAD_REQUEST;

        emit signalRecvUniterMessage(errorMessage);
    }

    // Ответ сервера на GET_KAFKA_CREDENTIALS: actual = true/false
    void emitOffsetCheckResponse(bool actual) {
        auto m = std::make_shared<contract::UniterMessage>();
        m->subsystem = contract::Subsystem::PROTOCOL;
        m->protact   = contract::ProtocolAction::GET_KAFKA_CREDENTIALS;
        m->status    = contract::MessageStatus::RESPONSE;
        m->error     = contract::ErrorCode::SUCCESS;
        m->add_data.emplace("offset_actual", actual ? "true" : "false");
        emit signalRecvUniterMessage(m);
    }

    // Ответ сервера о завершении FULL_SYNC
    void emitFullSyncDone() {
        auto m = std::make_shared<contract::UniterMessage>();
        m->subsystem = contract::Subsystem::PROTOCOL;
        m->protact   = contract::ProtocolAction::FULL_SYNC;
        m->status    = contract::MessageStatus::SUCCESS;
        emit signalRecvUniterMessage(m);
    }

public slots:
    // Слоты от AppManager
    void onMakeConnection() { ++makeConnectionCount; }
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> /*Message*/) {}
    void onPollMessages() { ++pollMessagesCount; }

    // Слоты MinIOConnector
    void onInitMinIO()       { ++initMinIOCount; }
    void onSubscribeKafka()  { ++subscribeKafkaCount; }

signals:
    void signalConnectionUpdated(bool state);
    void signalRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
};

// ============================================================================
// Mock ConfigManager
// ============================================================================
class MockConfigManager : public QObject {
    Q_OBJECT
public:
    MockConfigManager() = default;
    std::shared_ptr<contract::employees::Employee> m_User;
    void emitConfigured() { emit signalConfigured(); }
public slots:
    void onConfigProc(std::shared_ptr<contract::employees::Employee> User) {
        m_User = std::move(User);
    }
    void onClearResources() {
        m_User.reset();
    }
signals:
    void signalConfigured();
};

// ============================================================================
// Wrapper для AppManager: подмена внутренних связей на mock'и
// ============================================================================
class AppManagerTesting {
private:
    control::AppManager* appManager_;

public:
    AppManagerTesting() {
        appManager_ = control::AppManager::instance();
    }

    control::AppManager* get() { return appManager_; }

    void start_run() { appManager_->start_run(); }
    void onResourcesLoaded() { appManager_->onResourcesLoaded(); }
    void injectKafkaOffset(const QString& offset) {
        appManager_->onKafkaOffsetReceived(offset);
    }

    void replaceMockConfigManager(MockConfigManager* mock) {
        auto ConfigMgr = control::ConfigManager::instance();
        QObject::disconnect(ConfigMgr, nullptr, appManager_, nullptr);
        QObject::disconnect(appManager_, nullptr, ConfigMgr, nullptr);

        QObject::connect(mock, &MockConfigManager::signalConfigured,
                         appManager_, &control::AppManager::onConfigured);
        QObject::connect(appManager_, &control::AppManager::signalConfigProc,
                         mock, &MockConfigManager::onConfigProc);
        QObject::connect(appManager_, &control::AppManager::signalClearResources,
                         mock, &MockConfigManager::onClearResources);
    }
};


// ============================================================================
// Test fixture — переделан под новую FSM (KAFKA/SYNC + entry-actions)
// ============================================================================
class AppManagerTest : public ::testing::Test {
public:
    std::unique_ptr<AppManagerTesting> appManager;
    std::unique_ptr<MockAuthWidget>    mockAuthWidget;
    std::unique_ptr<MockNetwork>       mockNetwork;
    std::unique_ptr<MockConfigManager> mockConfigManager;

protected:
    void SetUp() override {
        appManager        = std::make_unique<AppManagerTesting>();
        mockAuthWidget    = std::make_unique<MockAuthWidget>();
        mockNetwork       = std::make_unique<MockNetwork>();
        mockConfigManager = std::make_unique<MockConfigManager>();

        appManager->replaceMockConfigManager(mockConfigManager.get());

        // === AppManager ↔ MockNetwork (сеть) ===
        QObject::connect(appManager->get(), &control::AppManager::signalSendUniterMessage,
                         mockNetwork.get(), &MockNetwork::onSendUniterMessage);
        QObject::connect(appManager->get(), &control::AppManager::signalMakeConnection,
                         mockNetwork.get(), &MockNetwork::onMakeConnection);
        QObject::connect(appManager->get(), &control::AppManager::signalPollMessages,
                         mockNetwork.get(), &MockNetwork::onPollMessages);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalRecvUniterMessage,
                         appManager->get(), &control::AppManager::onRecvUniterMessage);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalConnectionUpdated,
                         appManager->get(), &control::AppManager::onConnectionUpdated);

        // === AppManager ↔ MockNetwork (MinIOConnector-роль) ===
        QObject::connect(appManager->get(), &control::AppManager::signalInitMinIO,
                         mockNetwork.get(), &MockNetwork::onInitMinIO);
        QObject::connect(appManager->get(), &control::AppManager::signalSubscribeKafka,
                         mockNetwork.get(), &MockNetwork::onSubscribeKafka);

        // === AppManager ↔ AuthWidget ===
        QObject::connect(mockAuthWidget.get(), &MockAuthWidget::signalSendUniterMessage,
                         appManager->get(), &control::AppManager::onSendUniterMessage);
        QObject::connect(appManager->get(), &control::AppManager::signalFindAuthData,
                         mockAuthWidget.get(), &MockAuthWidget::onFindAuthData);
        QObject::connect(appManager->get(), &control::AppManager::signalAuthed,
                         mockAuthWidget.get(), &MockAuthWidget::onAuthed);
    }

    void TearDown() override {
        QObject::disconnect(appManager->get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockNetwork.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockAuthWidget.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockConfigManager.get(), nullptr, nullptr, nullptr);
    }

    // Утилита: довести FSM до состояния READY по «золотому пути».
    void driveToReady() {
        appManager->start_run();
        mockNetwork->emitConnectionUpdated(true);     // STARTED → AUTHENIFICATION
        mockAuthWidget->emitUserData();               // AUTHENIFICATION → IDLE_AUTHENIFICATION
        mockNetwork->emitAuthSuccess();               // IDLE_AUTHENIFICATION → DBLOADING
        appManager->onResourcesLoaded();              // DBLOADING → CONFIGURATING
        mockConfigManager->emitConfigured();          // CONFIGURATING → KAFKA
        appManager->injectKafkaOffset("offset-12345");// KAFKA: OFFSET_RECEIVED → запрос серверу
        mockNetwork->emitOffsetCheckResponse(true);   // KAFKA → READY (offset актуален)
    }
};

// ============================================================================
// 1. Базовая FSM: IDLE → STARTED → AUTHENIFICATION (online/offline)
// ============================================================================
TEST_F(AppManagerTest, BasicFSM_StartedAndAuthenification) {
    QSignalSpy spyMakeConnection(appManager->get(),
                                 &control::AppManager::signalMakeConnection);
    QSignalSpy spyConnectionUpdated(appManager->get(),
                                    &control::AppManager::signalConnectionUpdated);
    QSignalSpy spyFindAuthData(appManager->get(),
                               &control::AppManager::signalFindAuthData);

    // 1. START
    appManager->start_run();
    EXPECT_EQ(spyMakeConnection.count(), 1);

    // 2. Подключение сети — AUTHENIFICATION: запрошены auth-данные
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(spyConnectionUpdated.count(), 1);
    EXPECT_TRUE(spyConnectionUpdated.at(0).at(0).toBool());
    EXPECT_EQ(spyFindAuthData.count(), 1);

    // 3. Потеря сети → NetState меняется, логическое состояние сохраняется
    mockNetwork->emitConnectionUpdated(false);
    EXPECT_EQ(spyConnectionUpdated.count(), 2);
    EXPECT_FALSE(spyConnectionUpdated.at(1).at(0).toBool());

    // 4. Возврат сети — signalFindAuthData повторно не дёргается
    //    (логическое состояние не менялось)
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(spyConnectionUpdated.count(), 3);
    EXPECT_EQ(spyFindAuthData.count(), 1);
}

// ============================================================================
// 2. Плохой AUTH: IDLE_AUTHENIFICATION → AUTHENIFICATION (signalAuthed(false))
// ============================================================================
TEST_F(AppManagerTest, BadAuth_ReturnsToAuthenification) {
    QSignalSpy spyAuthed(appManager->get(), &control::AppManager::signalAuthed);
    QSignalSpy spySend  (appManager->get(), &control::AppManager::signalSendUniterMessage);
    QSignalSpy spyFindAuthData(appManager->get(), &control::AppManager::signalFindAuthData);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();   // → IDLE_AUTHENIFICATION
    EXPECT_EQ(spySend.count(), 1);

    // Плохой ответ → возврат в AUTHENIFICATION, запрос auth-данных снова
    mockNetwork->emitAuthError();
    EXPECT_EQ(spyAuthed.count(), 1);
    EXPECT_FALSE(spyAuthed.at(0).at(0).toBool());
    EXPECT_FALSE(mockAuthWidget->authed);
    // AUTHENIFICATION entry: либо signalFindAuthData, либо send сохранённого
    // authMessage. После AUTH_FAIL буфер сброшен, поэтому должен быть
    // signalFindAuthData.
    EXPECT_EQ(spyFindAuthData.count(), 2); // первый — после connect, второй — после fail
}

// ============================================================================
// 3. Хороший AUTH: IDLE_AUTHENIFICATION → DBLOADING
// ============================================================================
TEST_F(AppManagerTest, GoodAuth_TransitionsToDbLoading) {
    QSignalSpy spyAuthed(appManager->get(), &control::AppManager::signalAuthed);
    QSignalSpy spyLoadResources(appManager->get(), &control::AppManager::signalLoadResources);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();

    // DBLOADING entry: signalAuthed(true) + signalLoadResources(userhash)
    EXPECT_EQ(spyAuthed.count(), 1);
    EXPECT_TRUE(spyAuthed.at(0).at(0).toBool());
    EXPECT_TRUE(mockAuthWidget->authed);
    EXPECT_EQ(spyLoadResources.count(), 1);
}

// ============================================================================
// 4. KAFKA: offset → OFFSET_ACTUAL → READY (по «золотому пути»)
// ============================================================================
TEST_F(AppManagerTest, KafkaFlow_OffsetActual_GoesToReady) {
    QSignalSpy spyInitMinIO (appManager->get(), &control::AppManager::signalInitMinIO);
    QSignalSpy spySend      (appManager->get(), &control::AppManager::signalSendUniterMessage);
    QSignalSpy spySubscribe (appManager->get(), &control::AppManager::signalSubscribeKafka);
    QSignalSpy spyPoll      (appManager->get(), &control::AppManager::signalPollMessages);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();
    appManager->onResourcesLoaded();
    mockConfigManager->emitConfigured();

    // Вход в KAFKA: signalInitMinIO → MinIOConnector инициализируется
    EXPECT_EQ(spyInitMinIO.count(), 1);

    // OFFSET получен от MinIOConnector → AppManager шлёт запрос серверу
    const int sendCountBefore = spySend.count();
    appManager->injectKafkaOffset("offset-12345");
    EXPECT_EQ(spySend.count(), sendCountBefore + 1);

    // Сервер подтверждает актуальность → READY, подписка на Kafka
    mockNetwork->emitOffsetCheckResponse(true);
    EXPECT_EQ(spySubscribe.count(), 1);
    EXPECT_EQ(spyPoll.count(), 0);
}

// ============================================================================
// 5. KAFKA: offset stale → SYNC → READY
// ============================================================================
TEST_F(AppManagerTest, KafkaFlow_OffsetStale_GoesToSyncThenReady) {
    QSignalSpy spyPoll     (appManager->get(), &control::AppManager::signalPollMessages);
    QSignalSpy spySubscribe(appManager->get(), &control::AppManager::signalSubscribeKafka);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();
    appManager->onResourcesLoaded();
    mockConfigManager->emitConfigured();
    appManager->injectKafkaOffset("offset-outdated");

    // Сервер: offset устарел → SYNC (signalPollMessages при входе)
    mockNetwork->emitOffsetCheckResponse(false);
    EXPECT_EQ(spyPoll.count(), 1);
    EXPECT_EQ(spySubscribe.count(), 0);

    // Завершаем SYNC → READY
    mockNetwork->emitFullSyncDone();
    EXPECT_EQ(spySubscribe.count(), 1);
}

// ============================================================================
// 6. READY: CRUD-сообщения из Kafka пересылаются в DataManager
// ============================================================================
TEST_F(AppManagerTest, ReadyFlow_CrudForwardedDownstream) {
    QSignalSpy spyRecv(appManager->get(), &control::AppManager::signalRecvUniterMessage);

    driveToReady();

    // Kafka шлёт NOTIFICATION → forward в DataManager
    mockNetwork->emitCrudMessage();
    EXPECT_EQ(spyRecv.count(), 1);

    auto args = spyRecv.takeFirst();
    auto message = qvariant_cast<std::shared_ptr<contract::UniterMessage>>(args.at(0));
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->subsystem, contract::Subsystem::PURCHASES);
    EXPECT_EQ(message->crudact, contract::CrudAction::CREATE);
}

// ============================================================================
// 7. READY → LOGOUT → IDLE_AUTHENIFICATION (signalLoggedOut + очистка)
// ============================================================================
TEST_F(AppManagerTest, ReadyFlow_LogoutReturnsToIdleAuth) {
    QSignalSpy spyLogout(appManager->get(), &control::AppManager::signalLoggedOut);
    QSignalSpy spyClear (appManager->get(), &control::AppManager::signalClearResources);

    driveToReady();

    appManager->get()->onLogout();
    EXPECT_EQ(spyLogout.count(), 1);
    EXPECT_EQ(spyClear.count(), 1);
}


} // namespace appmanagertest

#include "appmanagertest.moc"
