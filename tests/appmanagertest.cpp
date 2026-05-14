#include "../src/uniter/control/appmanager.h"
#include "../src/uniter/control/configmanager.h"
#include "../src/uniter/contract/unitermessage.h"
#include "../src/uniter/contract/qt_compat.h"
#include "../src/uniter/contract/common/resourceabstract.h"
#include "../src/uniter/contract/manager/employee.h"
#include "../src/uniter/contract/supply/purchase.h"

#include <gtest/gtest.h>
#include <QObject>
#include <QSignalSpy>
#include <QCryptographicHash>
#include <QString>
#include <memory>


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
// Mock Network — имитирует и MockNetManager, и KafkaConnector.
// Это даёт тесту полный контроль над сценарием KCONNECTOR/KAFKA/DBCLEAR/SYNC/READY.
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

    // CRUD NOTIFICATION из "Kafka"
    void emitCrudMessage() {
        auto crudMessage = std::make_shared<contract::UniterMessage>();
        crudMessage->subsystem = contract::Subsystem::PURCHASES;
        crudMessage->crudact   = contract::CrudAction::CREATE;
        crudMessage->status    = contract::MessageStatus::NOTIFICATION;

        auto newPurch = std::make_shared<contract::supply::Purchase>(
            1001, true,
            contract::qt_compat::qDateTimeToTimePoint(QDateTime::currentDateTime()),
            contract::qt_compat::qDateTimeToTimePoint(QDateTime::currentDateTime()),
            12345, 12345,
            "Закупка стали 10т",
            "Нужна сталь для проекта #47",
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
            contract::qt_compat::qDateTimeToTimePoint(QDateTime::currentDateTime()),
            contract::qt_compat::qDateTimeToTimePoint(QDateTime::currentDateTime()),
            0, 0,
            "Иван", "Иванов", "Иванович",
            "ivan@company.ru",
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

    // Подсчёт исходящих UniterMessage определённого вида.
    int countSent(contract::ProtocolAction action, contract::MessageStatus status) const {
        int n = 0;
        for (auto& m : sentMessages) {
            if (!m) continue;
            if (m->protact == action && m->status == status) ++n;
        }
        return n;
    }

public slots:
    // Слоты от AppManager
    void onMakeConnection() { ++makeConnectionCount; }
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message) {
        sentMessages.push_back(std::move(Message));
    }

    // Слоты KafkaConnector
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
// Mock DataManager — под DBCLEAR / DBLOADING.
// ============================================================================
class MockDataManager : public QObject {
    Q_OBJECT
public:
    MockDataManager() = default;
    int clearDatabaseCount = 0;
    int loadResourcesCount = 0;

    void emitResourcesLoaded() { emit signalResourcesLoaded(); }
    void emitDatabaseCleared() { emit signalDatabaseCleared(); }

public slots:
    void onStartLoadResources(QByteArray /*userhash*/) { ++loadResourcesCount; }
    void onClearResources() {}
    void onClearDatabase() { ++clearDatabaseCount; }

signals:
    void signalResourcesLoaded();
    void signalDatabaseCleared();
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
// Test fixture — переделан под новую FSM (KCONNECTOR/DBCLEAR + entry-actions)
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

        // === AppManager ↔ MockNetwork (сеть) ===
        // После рефакторинга маршрутизации (docs/appmanager_routing.md)
        // AppManager разделил исходящие на signalSendToServer (сервер) и
        // signalSendToMinio (MinIO). В этих тестах CRUD/MINIO не используется,
        // все проверяемые исходящие — AUTH / GET_KAFKA_CREDENTIALS / FULL_SYNC —
        // уходят через signalSendToServer.
        QObject::connect(appManager->get(), &control::AppManager::signalSendToServer,
                         mockNetwork.get(), &MockNetwork::onSendUniterMessage);
        QObject::connect(appManager->get(), &control::AppManager::signalMakeConnection,
                         mockNetwork.get(), &MockNetwork::onMakeConnection);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalRecvUniterMessage,
                         appManager->get(), &control::AppManager::onRecvUniterMessage);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalConnectionUpdated,
                         appManager->get(), &control::AppManager::onConnectionUpdated);

        // === AppManager ↔ MockNetwork (KafkaConnector-роль) ===
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
                         mockDataManager.get(), &MockDataManager::onStartLoadResources);
        QObject::connect(mockDataManager.get(), &MockDataManager::signalResourcesLoaded,
                         appManager->get(), &control::AppManager::onResourcesLoaded);
        QObject::connect(appManager->get(), &control::AppManager::signalClearResources,
                         mockDataManager.get(), &MockDataManager::onClearResources);
        QObject::connect(appManager->get(), &control::AppManager::signalClearDatabase,
                         mockDataManager.get(), &MockDataManager::onClearDatabase);
        QObject::connect(mockDataManager.get(), &MockDataManager::signalDatabaseCleared,
                         appManager->get(), &control::AppManager::onDatabaseCleared);
    }

    void TearDown() override {
        QObject::disconnect(appManager->get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockNetwork.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockAuthWidget.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockConfigManager.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockDataManager.get(), nullptr, nullptr, nullptr);
    }

    // Утилита: довести FSM до состояния READY по «золотому пути».
    // KafkaConnector сигналит offset (имитация через injectKafkaOffset).
    void driveToReady() {
        appManager->start_run();
        mockNetwork->emitConnectionUpdated(true);     // STARTED → AUTHENIFICATION
        mockAuthWidget->emitUserData();               // AUTHENIFICATION → IDLE_AUTHENIFICATION
        mockNetwork->emitAuthSuccess();               // IDLE_AUTHENIFICATION → DBLOADING
        mockDataManager->emitResourcesLoaded();       // DBLOADING → CONFIGURATING
        mockConfigManager->emitConfigured();          // CONFIGURATING → KCONNECTOR
        appManager->injectKafkaOffset("offset-12345");// KCONNECTOR → KAFKA
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

    // 4. Возврат сети: AUTHENIFICATION — сетевое состояние, entry повторяется →
    //    signalFindAuthData дёрнется ещё раз.
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(spyConnectionUpdated.count(), 3);
    EXPECT_EQ(spyFindAuthData.count(), 2);
}

// ============================================================================
// 2. Плохой AUTH: IDLE_AUTHENIFICATION → AUTHENIFICATION (signalAuthed(false))
// ============================================================================
TEST_F(AppManagerTest, BadAuth_ReturnsToAuthenification) {
    QSignalSpy spyAuthed(appManager->get(), &control::AppManager::signalAuthed);
    QSignalSpy spySend  (appManager->get(), &control::AppManager::signalSendToServer);
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
    EXPECT_EQ(mockDataManager->loadResourcesCount, 1);
}

// ============================================================================
// 4. KCONNECTOR: инициализация KafkaConnector с userhash
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

    // Вход в KCONNECTOR: signalInitKafkaConnector(hash) к KafkaConnector.
    EXPECT_EQ(spyInitKafka.count(), 1);
    EXPECT_EQ(mockNetwork->initKafkaConnectorCount, 1);
    // Hash не пустой (для реального user с id=12345 это "12345").
    EXPECT_FALSE(mockNetwork->lastUserHash.isEmpty());
    EXPECT_EQ(mockNetwork->lastUserHash, QByteArray("12345"));
}

// ============================================================================
// 5. KAFKA: offset → OFFSET_ACTUAL → READY (по «золотому пути»)
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
    // В KAFKA entry: отправляется UniterMessage GET_KAFKA_CREDENTIALS REQUEST
    EXPECT_EQ(static_cast<int>(mockNetwork->sentMessages.size()), sendBefore + 1);
    EXPECT_EQ(mockNetwork->countSent(contract::ProtocolAction::GET_KAFKA_CREDENTIALS,
                                     contract::MessageStatus::REQUEST), 1);

    // Сервер подтверждает актуальность → READY, подписка на Kafka
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

    // Сервер: offset устарел → DBCLEAR (signalClearDatabase при входе)
    mockNetwork->emitOffsetCheckResponse(false);
    EXPECT_EQ(spyClearDb.count(), 1);
    EXPECT_EQ(mockDataManager->clearDatabaseCount, 1);

    // DataManager подтверждает очистку БД → SYNC: отправляется FULL_SYNC REQUEST
    mockDataManager->emitDatabaseCleared();
    EXPECT_EQ(mockNetwork->countSent(contract::ProtocolAction::FULL_SYNC,
                                     contract::MessageStatus::REQUEST), 1);
    EXPECT_EQ(spySubscribe.count(), 0);

    // Завершаем SYNC → READY
    mockNetwork->emitFullSyncDone();
    EXPECT_EQ(spySubscribe.count(), 1);
}

// ============================================================================
// 7. READY: CRUD-сообщения из Kafka пересылаются в DataManager
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
// 8. READY → LOGOUT → IDLE_AUTHENIFICATION (signalLoggedOut + очистка)
//    IDLE_AUTHENIFICATION — без entry-action, чтобы AuthWidget НЕ сделал
//    авто-логин сохранёнными логином/паролем (Remember me).
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
    // IDLE_AUTHENIFICATION не имеет entry-action — signalFindAuthData НЕ
    // должен вызываться, иначе AuthWidget сделает авто-логин.
    EXPECT_EQ(spyFindAuthData.count(), findBefore);
}

// ============================================================================
// 8б. READY → LOGOUT → повторный LOGIN полностью доходит до сервера.
// В IDLE_AUTHENIFICATION после LOGOUT m_authMessage сброшен — новый
// AUTH REQUEST от UI должен уйти в сеть, а не просто буферизоваться.
// ============================================================================
TEST_F(AppManagerTest, ReadyFlow_LogoutAllowsRelogin) {
    QSignalSpy spySend(appManager->get(), &control::AppManager::signalSendToServer);

    driveToReady();

    appManager->get()->onLogout();
    ASSERT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::IDLE_AUTHENIFICATION);

    const int sendBefore = spySend.count();

    // Повторный AUTH REQUEST от UI: должен уйти на сервер.
    mockAuthWidget->emitUserData();
    EXPECT_EQ(spySend.count(), sendBefore + 1);
    // Остаёмся в IDLE_AUTHENIFICATION ждать ответ сервера.
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::IDLE_AUTHENIFICATION);

    // Сервер возвращает успех — проходим дальше по FSM.
    mockNetwork->emitAuthSuccess();
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::DBLOADING);

    // Спам-протект: второй AUTH REQUEST во время ожидания не
    // должен уйти в сеть (проверяем отдельно).
}

// ============================================================================
// 8в. IDLE_AUTHENIFICATION — второй AUTH REQUEST во время ожидания
//      не уходит в сеть (защита от спама).
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

    // Второй AUTH REQUEST пока ждём ответ — не должен уйти.
    mockAuthWidget->emitUserData();
    EXPECT_EQ(spySend.count(), sendAfterFirst);
}

// ============================================================================
// 9. KCONNECTOR — локальное состояние: потеря/возврат сети НЕ повторяет entry.
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

    // Потеря сети в KCONNECTOR (локальное состояние)
    mockNetwork->emitConnectionUpdated(false);
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::KCONNECTOR);
    EXPECT_EQ(appManager->get()->currentNetState(),
              control::AppManager::NetState::OFFLINE);

    // Возврат сети — entry НЕ повторяется для KCONNECTOR.
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(spyInitKafka.count(), 1); // по-прежнему 1
}

// ============================================================================
// 10. KAFKA — сетевое состояние: при потере сети entry не стартует,
//     при возврате online — повторяется.
// ============================================================================
TEST_F(AppManagerTest, KafkaFlow_DeferredWhenOffline_RepeatsOnReconnect) {
    appManager->start_run();
    mockNetwork->emitConnectionUpdated(true);
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();
    mockDataManager->emitResourcesLoaded();
    mockConfigManager->emitConfigured();     // → KCONNECTOR

    // Имитируем потерю сети ДО того как KafkaConnector прислал offset.
    mockNetwork->emitConnectionUpdated(false);
    EXPECT_EQ(appManager->get()->currentNetState(),
              control::AppManager::NetState::OFFLINE);

    // KafkaConnector (локально!) инициализировался и прислал offset в OFFLINE —
    // FSM перейдёт в KAFKA, но entry (сетевой запрос) отложится.
    const int sendBefore = mockNetwork->sentMessages.size();
    appManager->injectKafkaOffset("offset-xyz");
    EXPECT_EQ(appManager->get()->currentState(),
              control::AppManager::AppState::KAFKA);
    EXPECT_EQ(static_cast<int>(mockNetwork->sentMessages.size()), sendBefore); // запрос не ушёл

    // Возврат сети — entry KAFKA повторяется: запрос GET_KAFKA_CREDENTIALS уходит.
    mockNetwork->emitConnectionUpdated(true);
    EXPECT_EQ(mockNetwork->countSent(contract::ProtocolAction::GET_KAFKA_CREDENTIALS,
                                     contract::MessageStatus::REQUEST), 1);
}


} // namespace appmanagertest

#include "appmanagertest.moc"
