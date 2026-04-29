#include "../src/uniter/control/appmanager.h"
#include "../src/uniter/control/configmanager.h"
#include "../src/uniter/contract/unitermessage.h"
#include "../src/uniter/contract/resourceabstract.h"
#include "../src/uniter/contract/manager/employee.h"
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
// Mock AuthWidget - simulates a custom login widget.
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
        // In reality, show the login screen. We do nothing for the test.
    }
    void onAuthed(bool result) {
        authed = result;
        if (!result) ++authedFalseCount;
    }
signals:
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
};

// ============================================================================
// Mock Network - Simulates both MockNetManager and KafkaConnector.
// This gives the test full control over the KCONNECTOR/KAFKA/DBCLEAR/SYNC/READY script.
// ============================================================================
class MockNetwork : public QObject {
    Q_OBJECT
public:
    MockNetwork() = default;

    int subscribeKafkaCount    = 0;
    int initKafkaConnectorCount = 0;
    int makeConnectionCount    = 0;

    QByteArray lastUserHash;
    std::vector<std::shared_ptr<contract::UniterMessage>> sentMessages;

    void emitConnectionUpdated(bool state) {
        emit signalConnectionUpdated(state);
    }

    // CRUD NOTIFICATION from "Kafka"
    void emitCrudMessage() {
        auto crudMessage = std::make_shared<contract::UniterMessage>();
        crudMessage->subsystem = contract::Subsystem::PURCHASES;
        crudMessage->crudact   = contract::CrudAction::CREATE;
        crudMessage->status    = contract::MessageStatus::NOTIFICATION;

        auto newPurch = std::make_shared<contract::supply::Purchase>(
            1001, true,
            QDateTime::currentDateTime(), QDateTime::currentDateTime(),
            12345, 12345,
            QString("Steel purchase 10t"),
            QString("Steel is needed for project #47"),
            contract::supply::PurchStatus::DRAFT,
            std::nullopt);
        crudMessage->resource = std::move(newPurch);

        emit signalRecvUniterMessage(std::move(crudMessage));
    }

    // AUTH RESPONSE success (with Employee)
    void emitAuthSuccess() {
        auto authMessageSuccess = std::make_shared<contract::UniterMessage>();
        authMessageSuccess->subsystem = contract::Subsystem::PROTOCOL;
        authMessageSuccess->protact   = contract::ProtocolAction::AUTH;
        authMessageSuccess->status    = contract::MessageStatus::RESPONSE;
        authMessageSuccess->error     = contract::ErrorCode::SUCCESS;

        std::vector<contract::manager::EmployeeAssignment> assignments;
        auto employee = std::make_shared<contract::manager::Employee>(
            12345, true,
            QDateTime::currentDateTime(), QDateTime::currentDateTime(),
            0, 0,
            QString("Ivan"), QString("Ivanov"), QString("Ivanovich"),
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

    // Server response to GET_KAFKA_CREDENTIALS: actual = true/false
    void emitOffsetCheckResponse(bool actual) {
        auto m = std::make_shared<contract::UniterMessage>();
        m->subsystem = contract::Subsystem::PROTOCOL;
        m->protact   = contract::ProtocolAction::GET_KAFKA_CREDENTIALS;
        m->status    = contract::MessageStatus::RESPONSE;
        m->error     = contract::ErrorCode::SUCCESS;
        m->add_data.emplace("offset_actual", actual ? "true" : "false");
        emit signalRecvUniterMessage(m);
    }

    // Server response about completion of FULL_SYNC
    void emitFullSyncDone() {
        auto m = std::make_shared<contract::UniterMessage>();
        m->subsystem = contract::Subsystem::PROTOCOL;
        m->protact   = contract::ProtocolAction::FULL_SYNC;
        m->status    = contract::MessageStatus::SUCCESS;
        emit signalRecvUniterMessage(m);
    }

    // Counting outgoing UniterMessages of a certain type.
    int countSent(contract::ProtocolAction action, contract::MessageStatus status) const {
        int n = 0;
        for (auto& m : sentMessages) {
            if (!m) continue;
            if (m->protact == action && m->status == status) ++n;
        }
        return n;
    }

public slots:
    // Slots from AppManager
    void onMakeConnection() { ++makeConnectionCount; }
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message) {
        sentMessages.push_back(std::move(Message));
    }

    // KafkaConnector slots
    void onInitKafkaConnector(QByteArray userhash) {
        ++initKafkaConnectorCount;
        lastUserHash = std::move(userhash);
    }
    void onSubscribeKafka() { ++subscribeKafkaCount; }

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
    std::shared_ptr<contract::manager::Employee> m_User;
    void emitConfigured() { emit signalConfigured(); }
public slots:
    void onConfigProc(std::shared_ptr<contract::manager::Employee> User) {
        m_User = std::move(User);
    }
    void onClearResources() {
        m_User.reset();
    }
signals:
    void signalConfigured();
};

// ============================================================================
// Mock DataManager - under DBCLEAR / DBLOADING.
// ============================================================================
class MockDataManager : public QObject {
    Q_OBJECT
public:
    MockDataManager() = default;
    int clearDatabaseCount = 0;
    int loadResourcesCount = 0;

    void emitResourcesLoaded() { emit signalResourcesLoaded(); }
    void emitResourcesCleared() { emit signalResourcesCleared(); }

public slots:
    void onInitDatabase(QByteArray /*userhash*/) { ++loadResourcesCount; }
    void onResetDatabase() {}
    void onClearResourcesForSync() { ++clearDatabaseCount; }

signals:
    void signalResourcesLoaded();
    void signalResourcesCleared();
};

// ============================================================================
// Wrapper for AppManager: replacing internal connections with mocks
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
// Test fixture - redesigned for the new FSM (KCONNECTOR/DBCLEAR + entry-actions)
// ============================================================================
class AppManagerTest : public ::testing::Test {
public:
    std::unique_ptr<AppManagerTesting> appManager;
    std::unique_ptr<MockAuthWidget>    mockAuthWidget;
    std::unique_ptr<MockNetwork>       mockNetwork;
    std::unique_ptr<MockConfigManager> mockConfigManager;
    std::unique_ptr<MockDataManager>   mockDataManager;

protected:
    void SetUp() override {
        appManager        = std::make_unique<AppManagerTesting>();
        mockAuthWidget    = std::make_unique<MockAuthWidget>();
        mockNetwork       = std::make_unique<MockNetwork>();
        mockConfigManager = std::make_unique<MockConfigManager>();
        mockDataManager   = std::make_unique<MockDataManager>();

        appManager->get()->resetForTest();

        appManager->replaceMockConfigManager(mockConfigManager.get());

        // === AppManager ↔ MockNetwork (network) ===
        // After refactoring routing (docs/appmanager_routing.md)
        // AppManager split outgoing into signalSendToServer (server) and
        // signalSendToMinio(MinIO). CRUD/MINIO is not used in these tests,
        // all outgoing scans - AUTH / GET_KAFKA_CREDENTIALS / FULL_SYNC -
        // leave via signalSendToServer.
        QObject::connect(appManager->get(), &control::AppManager::signalSendToServer,
                         mockNetwork.get(), &MockNetwork::onSendUniterMessage);
        QObject::connect(appManager->get(), &control::AppManager::signalMakeConnection,
                         mockNetwork.get(), &MockNetwork::onMakeConnection);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalRecvUniterMessage,
                         appManager->get(), &control::AppManager::onRecvUniterMessage);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalConnectionUpdated,
                         appManager->get(), &control::AppManager::onConnectionUpdated);

        // === AppManager ↔ MockNetwork (KafkaConnector role) ===
        QObject::connect(appManager->get(), &control::AppManager::signalInitKafkaConnector,
                         mockNetwork.get(), &MockNetwork::onInitKafkaConnector);
        QObject::connect(appManager->get(), &control::AppManager::signalSubscribeKafka,
                         mockNetwork.get(), &MockNetwork::onSubscribeKafka);

        // === AppManager ↔ AuthWidget ===
        QObject::connect(mockAuthWidget.get(), &MockAuthWidget::signalSendUniterMessage,
                         appManager->get(), &control::AppManager::onSendUniterMessage);
        QObject::connect(appManager->get(), &control::AppManager::signalFindAuthData,
                         mockAuthWidget.get(), &MockAuthWidget::onFindAuthData);
        QObject::connect(appManager->get(), &control::AppManager::signalAuthed,
                         mockAuthWidget.get(), &MockAuthWidget::onAuthed);

        // === AppManager ↔ MockDataManager (DBLOADING / DBCLEAR) ===
        QObject::connect(appManager->get(), &control::AppManager::signalLoadResources,
                         mockDataManager.get(), &MockDataManager::onInitDatabase);
        QObject::connect(mockDataManager.get(), &MockDataManager::signalResourcesLoaded,
                         appManager->get(), &control::AppManager::onResourcesLoaded);
        QObject::connect(appManager->get(), &control::AppManager::signalClearResources,
                         mockDataManager.get(), &MockDataManager::onResetDatabase);
        QObject::connect(appManager->get(), &control::AppManager::signalClearDatabase,
                         mockDataManager.get(), &MockDataManager::onClearResourcesForSync);
        QObject::connect(mockDataManager.get(), &MockDataManager::signalResourcesCleared,
                         appManager->get(), &control::AppManager::onDatabaseCleared);
    }

    void TearDown() override {
        QObject::disconnect(appManager->get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockNetwork.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockAuthWidget.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockConfigManager.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockDataManager.get(), nullptr, nullptr, nullptr);
    }

    // Utility: bring FSM to READY state along the “golden path”.
    // KafkaConnector signals offset (imitation via injectKafkaOffset).
    void driveToReady() {
        appManager->start_run();
        mockNetwork->emitConnectionUpdated(true);     // STARTED → AUTHENIFICATION
        mockAuthWidget->emitUserData();               // AUTHENIFICATION → IDLE_AUTHENIFICATION
        mockNetwork->emitAuthSuccess();               // IDLE_AUTHENIFICATION → DBLOADING
        mockDataManager->emitResourcesLoaded();       // DBLOADING → CONFIGURATING
        mockConfigManager->emitConfigured();          // CONFIGURATING → KCONNECTOR
        appManager->injectKafkaOffset("offset-12345");// KCONNECTOR → KAFKA
        mockNetwork->emitOffsetCheckResponse(true);   // KAFKA → READY (offset is relevant)
    }
};

// ============================================================================
// 1. Basic FSM: IDLE → STARTED → AUTHENIFICATION (online/offline)
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

    // 2. Network connection - AUTHENIFICATION: auth data requested
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(spyConnectionUpdated.count(), 1);
    EXPECT_TRUE(spyConnectionUpdated.at(0).at(0).toBool());
    EXPECT_EQ(spyFindAuthData.count(), 1);

    // 3. Network loss → NetState changes, logical state remains
    mockNetwork->emitConnectionUpdated(false);
    EXPECT_EQ(spyConnectionUpdated.count(), 2);
    EXPECT_FALSE(spyConnectionUpdated.at(1).at(0).toBool());

    // 4. Network return: AUTHENIFICATION - network state, entry is repeated →
    // signalFindAuthData will twitch again.
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(spyConnectionUpdated.count(), 3);
    EXPECT_EQ(spyFindAuthData.count(), 2);
}

// ============================================================================
// 2. Bad AUTH: IDLE_AUTHENIFICATION → AUTHENIFICATION (signalAuthed(false))
// ============================================================================
TEST_F(AppManagerTest, BadAuth_ReturnsToAuthenification) {
    QSignalSpy spyAuthed(appManager->get(), &control::AppManager::signalAuthed);
    QSignalSpy spySend  (appManager->get(), &control::AppManager::signalSendToServer);
    QSignalSpy spyFindAuthData(appManager->get(), &control::AppManager::signalFindAuthData);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();   // → IDLE_AUTHENIFICATION
    EXPECT_EQ(spySend.count(), 1);

    // Bad response → return to AUTHENIFICATION, request auth data again
    mockNetwork->emitAuthError();
    EXPECT_EQ(spyAuthed.count(), 1);
    EXPECT_FALSE(spyAuthed.at(0).at(0).toBool());
    EXPECT_FALSE(mockAuthWidget->authed);
    // AUTHENIFICATION entry: either signalFindAuthData or send saved
    // authMessage. After AUTH_FAIL the buffer is reset, so it must be
    // signalFindAuthData.
    EXPECT_EQ(spyFindAuthData.count(), 2); // the first one is after connect, the second one is after fail
}

// ============================================================================
// 3. Good AUTH: IDLE_AUTHENIFICATION → DBLOADING
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
    EXPECT_EQ(mockDataManager->loadResourcesCount, 1);
}

// ============================================================================
// 4. KCONNECTOR: initializing KafkaConnector with userhash
// ============================================================================
TEST_F(AppManagerTest, KconnectorFlow_InitKafkaWithUserHash) {
    QSignalSpy spyInitKafka(appManager->get(),
                            &control::AppManager::signalInitKafkaConnector);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();
    mockDataManager->emitResourcesLoaded();
    mockConfigManager->emitConfigured();          // CONFIGURATING → KCONNECTOR

    // Input to KCONNECTOR: signalInitKafkaConnector(hash) to KafkaConnector.
    EXPECT_EQ(spyInitKafka.count(), 1);
    EXPECT_EQ(mockNetwork->initKafkaConnectorCount, 1);
    // Hash is not empty (for a real user with id=12345 it is "12345").
    EXPECT_FALSE(mockNetwork->lastUserHash.isEmpty());
    EXPECT_EQ(mockNetwork->lastUserHash, QByteArray("12345"));
}

// ============================================================================
// 5. KAFKA: offset → OFFSET_ACTUAL → READY (along the “golden path”)
// ============================================================================
TEST_F(AppManagerTest, KafkaFlow_OffsetActual_GoesToReady) {
    QSignalSpy spySubscribe (appManager->get(), &control::AppManager::signalSubscribeKafka);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();
    mockDataManager->emitResourcesLoaded();
    mockConfigManager->emitConfigured();     // → KCONNECTOR

    const int sendBefore = mockNetwork->sentMessages.size();
    appManager->injectKafkaOffset("offset-12345"); // KCONNECTOR → KAFKA
    // In KAFKA entry: UniterMessage GET_KAFKA_CREDENTIALS REQUEST is sent
    EXPECT_EQ(static_cast<int>(mockNetwork->sentMessages.size()), sendBefore + 1);
    EXPECT_EQ(mockNetwork->countSent(contract::ProtocolAction::GET_KAFKA_CREDENTIALS,
                                     contract::MessageStatus::REQUEST), 1);

    // The server confirms relevance → READY, subscription to Kafka
    mockNetwork->emitOffsetCheckResponse(true);
    EXPECT_EQ(spySubscribe.count(), 1);
    EXPECT_EQ(mockNetwork->subscribeKafkaCount, 1);
}

// ============================================================================
// 6. KAFKA: offset stale → DBCLEAR → SYNC → READY
// ============================================================================
TEST_F(AppManagerTest, KafkaFlow_OffsetStale_GoesToDbClearSyncReady) {
    QSignalSpy spyClearDb   (appManager->get(), &control::AppManager::signalClearDatabase);
    QSignalSpy spySubscribe (appManager->get(), &control::AppManager::signalSubscribeKafka);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();
    mockDataManager->emitResourcesLoaded();
    mockConfigManager->emitConfigured();
    appManager->injectKafkaOffset("offset-outdated");   // → KAFKA

    // Server: offset is deprecated -> DBCLEAR(signalClearDatabase on login)
    mockNetwork->emitOffsetCheckResponse(false);
    EXPECT_EQ(spyClearDb.count(), 1);
    EXPECT_EQ(mockDataManager->clearDatabaseCount, 1);

    // DataManager confirms database cleanup → SYNC: FULL_SYNC REQUEST is sent
    mockDataManager->emitResourcesCleared();
    EXPECT_EQ(mockNetwork->countSent(contract::ProtocolAction::FULL_SYNC,
                                     contract::MessageStatus::REQUEST), 1);
    EXPECT_EQ(spySubscribe.count(), 0);

    // Finish SYNC → READY
    mockNetwork->emitFullSyncDone();
    EXPECT_EQ(spySubscribe.count(), 1);
}

// ============================================================================
// 7. READY: CRUD messages from Kafka are forwarded to the DataManager
// ============================================================================
TEST_F(AppManagerTest, ReadyFlow_CrudForwardedDownstream) {
    QSignalSpy spyRecv(appManager->get(), &control::AppManager::signalRecvUniterMessage);

    driveToReady();

    // Kafka sends NOTIFICATION → forward to DataManager
    mockNetwork->emitCrudMessage();
    EXPECT_EQ(spyRecv.count(), 1);

    auto args = spyRecv.takeFirst();
    auto message = qvariant_cast<std::shared_ptr<contract::UniterMessage>>(args.at(0));
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->subsystem, contract::Subsystem::PURCHASES);
    EXPECT_EQ(message->crudact, contract::CrudAction::CREATE);
}

// ============================================================================
// 8. READY → LOGOUT → IDLE_AUTHENIFICATION (signalLoggedOut + cleanup)
// IDLE_AUTHENIFICATION - without entry-action so that AuthWidget does NOT do
// auto-login with saved login/password (Remember me).
// ============================================================================
TEST_F(AppManagerTest, ReadyFlow_LogoutReturnsToIdleAuth) {
    QSignalSpy spyLogout      (appManager->get(), &control::AppManager::signalLoggedOut);
    QSignalSpy spyClear       (appManager->get(), &control::AppManager::signalClearResources);
    QSignalSpy spyFindAuthData(appManager->get(), &control::AppManager::signalFindAuthData);

    driveToReady();

    const int findBefore = spyFindAuthData.count();

    appManager->get()->onLogout();
    EXPECT_EQ(spyLogout.count(), 1);
    EXPECT_EQ(spyClear.count(), 1);
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::IDLE_AUTHENIFICATION);
    // IDLE_AUTHENIFICATION does not have entry-action - signalFindAuthData does NOT
    // must be called, otherwise AuthWidget will auto-login.
    EXPECT_EQ(spyFindAuthData.count(), findBefore);
}

// ============================================================================
// 8b. READY → LOGOUT → repeated LOGIN completely reaches the server.
// In IDLE_AUTHENIFICATION after LOGOUT m_authMessage is reset - new
// AUTH REQUEST from the UI should go to the network, and not just be buffered.
// ============================================================================
TEST_F(AppManagerTest, ReadyFlow_LogoutAllowsRelogin) {
    QSignalSpy spySend(appManager->get(), &control::AppManager::signalSendToServer);

    driveToReady();

    appManager->get()->onLogout();
    ASSERT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::IDLE_AUTHENIFICATION);

    const int sendBefore = spySend.count();

    // Repeated AUTH REQUEST from UI: must go to the server.
    mockAuthWidget->emitUserData();
    EXPECT_EQ(spySend.count(), sendBefore + 1);
    // We remain in IDLE_AUTHENIFICATION to wait for the server’s response.
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::IDLE_AUTHENIFICATION);

    // The server returns success - we go further through the FSM.
    mockNetwork->emitAuthSuccess();
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::DBLOADING);

    // Spam protection: second AUTH REQUEST while waiting is not
    // should go online (check separately).
}

// ============================================================================
// 8th century IDLE_AUTHENIFICATION - second AUTH REQUEST while waiting
// does not go online (spam protection).
// ============================================================================
TEST_F(AppManagerTest, IdleAuth_DuplicateAuthRequest_Ignored) {
    QSignalSpy spySend(appManager->get(), &control::AppManager::signalSendToServer);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();   // AUTHENIFICATION → IDLE_AUTHENIFICATION
    ASSERT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::IDLE_AUTHENIFICATION);
    const int sendAfterFirst = spySend.count();
    ASSERT_GE(sendAfterFirst, 1);

    // The second AUTH REQUEST is still waiting for a response - it should not go away.
    mockAuthWidget->emitUserData();
    EXPECT_EQ(spySend.count(), sendAfterFirst);
}

// ============================================================================
// 9. KCONNECTOR - local state: network loss/return does NOT repeat entry.
// ============================================================================
TEST_F(AppManagerTest, KconnectorFlow_NetReconnect_DoesNotReinit) {
    QSignalSpy spyInitKafka(appManager->get(),
                            &control::AppManager::signalInitKafkaConnector);

    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();
    mockDataManager->emitResourcesLoaded();
    mockConfigManager->emitConfigured();     // → KCONNECTOR
    EXPECT_EQ(spyInitKafka.count(), 1);
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::KCONNECTOR);

    // Network loss in KCONNECTOR (local state)
    mockNetwork->emitConnectionUpdated(false);
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::KCONNECTOR);
    EXPECT_EQ(appManager->get()->currentNetState(),
              control::AppManager::NetState::OFFLINE);

    // Return network - entry is NOT repeated for KCONNECTOR.
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(spyInitKafka.count(), 1); // still 1
}

// ============================================================================
// 10. KAFKA - network state: if the network is lost, entry does not start,
// when returning online - repeats.
// ============================================================================
TEST_F(AppManagerTest, KafkaFlow_DeferredWhenOffline_RepeatsOnReconnect) {
    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();
    mockDataManager->emitResourcesLoaded();
    mockConfigManager->emitConfigured();     // → KCONNECTOR

    // We simulate network loss BEFORE KafkaConnector sends offset.
    mockNetwork->emitConnectionUpdated(false);
    EXPECT_EQ(appManager->get()->currentNetState(),
              control::AppManager::NetState::OFFLINE);

    // KafkaConnector (locally!) initialized and sent offset to OFFLINE -
    // FSM will go to KAFKA, but entry (network request) will be delayed.
    const int sendBefore = mockNetwork->sentMessages.size();
    appManager->injectKafkaOffset("offset-xyz");
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::KAFKA);
    EXPECT_EQ(static_cast<int>(mockNetwork->sentMessages.size()), sendBefore); // the request did not go through

    // Network return - entry KAFKA is repeated: the GET_KAFKA_CREDENTIALS request goes away.
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(mockNetwork->countSent(contract::ProtocolAction::GET_KAFKA_CREDENTIALS,
                                     contract::MessageStatus::REQUEST), 1);
}


} // namespace appmanagertest

#include "appmanagertest.moc"
