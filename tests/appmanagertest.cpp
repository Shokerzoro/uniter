
#include "../src/uniter/managers/appmanager.h"
#include "../src/uniter/messages/unitermessage.h"
#include "../src/uniter/resources/resourceabstract.h"
#include "../src/uniter/resources/employee/employee.h"
#include "../src/uniter/resources/supply/purchase.h"

#include <gtest/gtest.h>
#include <QObject>
#include <QSignalSpy>
#include <memory>

using namespace uniter;

namespace appmanagertest {


class MockAuthWidget : public QObject {
    Q_OBJECT
public:
    MockAuthWidget() = default;
    bool authed = false;
    void emitUserData() {
        auto authReqMessage = std::make_shared<messages::UniterMessage>();
        authReqMessage->subsystem = messages::Subsystem::PROTOCOL;
        authReqMessage->protact = messages::ProtocolAction::GETCONFIG;
        authReqMessage->add_data.emplace("login", "niger");
        authReqMessage->add_data.emplace("password", "12345");
        emit signalSendUniterMessage(authReqMessage);
    }
public slots:
    void onFindAuthData() {

    }
    void onAuthed(bool result) {
        // Как-то реагируем
        authed = result;
    }
signals:
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
};

class MockNetwork : public QObject {
    Q_OBJECT
public:
    MockNetwork() = default;

    void emitConnected() { emit signalConnected(); }
    void emitDisconnected() { emit signalDisconnected(); }

    // Отправка crud сообщения
    void emitCrudMessage() {
        auto crudMessage = std::make_shared<messages::UniterMessage>();
        crudMessage->subsystem = messages::Subsystem::PURCHASES;
        crudMessage->crudact = messages::CrudAction::CREATE;
        crudMessage->status = messages::MessageStatus::NOTIFICATION;

        auto newPurch = std::make_shared<resources::supply::Purchase>(
            1001,                                    // id
            true,                                    // actual
            QDateTime::currentDateTime(),            // created_at
            QDateTime::currentDateTime(),            // updated_at
            12345,                                   // created_by (User ID)
            12345,                                   // updated_by
            QString("Закупка стали 10т"),            // name
            QString("Нужна сталь для проекта #47"),  // description
            resources::supply::PurchStatus::DRAFT,                      // status
            nullptr                                  // material (пока null)
            );
        crudMessage->resource = std::move(newPurch);

        emit signalRecvUniterMessage(std::move(crudMessage));
    }

    // Отправляем ответы на запрос об авторизации
    void emitAuthSuccess() {

        auto authMessageSuccess = std::make_shared<messages::UniterMessage>();
        // Настройка успешного ответа аутентификации
        authMessageSuccess->subsystem = messages::Subsystem::PROTOCOL;
        authMessageSuccess->protact = messages::ProtocolAction::GETCONFIG;
        authMessageSuccess->status = messages::MessageStatus::RESPONSE;
        // Создаем тестового пользователя
        std::vector<resources::employees::EmployeeAssignment> assignments;

        // Employee: 10 аргументов
        auto employee = std::make_shared<resources::employees::Employee>(
            12345,                                   // id
            true,                                    // is_actual
            QDateTime::currentDateTime(),            // created_at
            QDateTime::currentDateTime(),            // updated_at
            0,                                       // created_by
            0,                                       // updated_by
            QString("Иван"),                         // name
            QString("Иванов"),                       // surname
            QString("Иванович"),                     // patronymic
            QString("ivan@company.ru"),              // email
            std::move(assignments)                   // assignments
            );
        authMessageSuccess->resource = std::move(employee);

        emit signalRecvUniterMessage(std::move(authMessageSuccess));
    }

    void emitAuthError() {

        auto errorMessage = std::make_shared<messages::UniterMessage>();
        // Настройка сообщения об ошибке аутентификации
        errorMessage->version = 1;
        errorMessage->subsystem = messages::Subsystem::PROTOCOL;
        errorMessage->protact = messages::ProtocolAction::GETCONFIG;
        errorMessage->status = messages::MessageStatus::RESPONSE;
        errorMessage->error = messages::ErrorCode::BAD_REQUEST;

        emit signalRecvUniterMessage(errorMessage);
    }

public slots:
    // Запрос на подключение
    void onMakeConnection(void) {

    }
    // Получение сообщения от AppManager
    void onSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message) {
        // Логируем или обрабатываем

    };

signals:
    void signalConnected();
    void signalDisconnected();
    // Получение сообщения из сети
    void signalRecvUniterMessage(std::shared_ptr<messages::UniterMessage> Message);
};

class MockConfigManager : public QObject {
    Q_OBJECT;
public:
    MockConfigManager() = default;
    std::shared_ptr<resources::employees::Employee> m_User;
    void emitConfigured() {
        emit signalConfigurated();
    }
public slots:
    void onConfigProc(std::shared_ptr<resources::employees::Employee> User) {
        // Копируем User
        m_User = std::move(User);
    }
signals:
    void signalConfigurated();
};

class MockLocalManager : public QObject {
    Q_OBJECT
public:
    MockLocalManager() = default;
    void emitCustomized() {
        emit signalCustomized();
    }
public slots:
    void onCustimizeProc() {}
signals:
    void signalCustomized();
};

// Делаем доступными приватные члены для тестирования
class AppManagerTesting : public managers::AppManager {
public:
    AppManagerTesting() = default;

    // Доступ к состоянию приложения
    uniter::managers::AppManager::AppState getAppState() const {
        return AState;
    }
    uniter::managers::AppManager::NetState getNetState() const {
        return NState;
    }
    // Методы проверки состояний
    bool isIdle() const { return AState == uniter::managers::AppManager::AppState::IDLE; }
    bool isStarted() const { return AState == uniter::managers::AppManager::AppState::STARTED; }
    bool isConnected() const { return AState == uniter::managers::AppManager::AppState::CONNECTED; }
    bool isAuthenticated() const { return AState == uniter::managers::AppManager::AppState::AUTHENTIFICATED; }
    bool isConfigurated() const { return AState == uniter::managers::AppManager::AppState::CONFIGURATED; }
    bool isReady() const { return AState == uniter::managers::AppManager::AppState::READY; }
    bool isShutdown() const { return AState == uniter::managers::AppManager::AppState::SHUTDOWN; }

    bool isOnline() const { return NState == uniter::managers::AppManager::NetState::ONLINE; }
    bool isOffline() const { return NState == uniter::managers::AppManager::NetState::OFFLINE; }
};

// Создаем и линкуем объекты
class AppManagerTest : public ::testing::Test {
public:
    std::unique_ptr<AppManagerTesting> appManager;
    std::unique_ptr<MockAuthWidget> mockAuthWidget;
    std::unique_ptr<MockNetwork> mockNetwork;
    std::unique_ptr<MockConfigManager> mockConfigManager;
    std::unique_ptr<MockLocalManager> mockLocalManager;
protected:
    void SetUp() override {
        // Создаем все объекты сначала
        appManager = std::make_unique<AppManagerTesting>();
        mockAuthWidget = std::make_unique<MockAuthWidget>();
        mockNetwork = std::make_unique<MockNetwork>();
        mockConfigManager = std::make_unique<MockConfigManager>();
        mockLocalManager = std::make_unique<MockLocalManager>();

        // Теперь линкуем сигналы и слоты (connect)
        // appmanager и сетевой класс
        QObject::connect(appManager.get(), &managers::AppManager::signalSendUniterMessage,
                mockNetwork.get(), &MockNetwork::onSendUniterMessage);
        QObject::connect(appManager.get(), &managers::AppManager::signalMakeConnection,
                mockNetwork.get(), &MockNetwork::onMakeConnection);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalRecvUniterMessage,
                appManager.get(), &managers::AppManager::onRecvUniterMessage);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalConnected,
                appManager.get(), &managers::AppManager::onConnected);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalDisconnected,
                appManager.get(), &managers::AppManager::onDisconnected);


        // appmanager и виджет аутентификации
        QObject::connect(mockAuthWidget.get(), &MockAuthWidget::signalSendUniterMessage,
                         appManager.get(), &managers::AppManager::onSendUniterMessage);
        QObject::connect(appManager.get(), &managers::AppManager::signalFindAuthData,
                         mockAuthWidget.get(), &MockAuthWidget::onFindAuthData);
        QObject::connect(appManager.get(), &managers::AppManager::signalAuthed,
                         mockAuthWidget.get(), &MockAuthWidget::onAuthed);

        // appmanager и менеджер конфигураций
        QObject::connect(mockConfigManager.get(), &MockConfigManager::signalConfigurated,
                         appManager.get(), &managers::AppManager::onConfigured);
        QObject::connect(appManager.get(), &managers::AppManager::signalConfigProc,
                         mockConfigManager.get(), &MockConfigManager::onConfigProc);


        // appmanager и менеджер локальных настроек
        QObject::connect(mockLocalManager.get(), &MockLocalManager::signalCustomized,
                         appManager.get(), &managers::AppManager::onCustomized);
        QObject::connect(appManager.get(), &managers::AppManager::signalCustomizeProc,
                         mockLocalManager.get(), &MockLocalManager::onCustimizeProc);
    }
};

// Тут различные сценарии тестирования

// Базовый прогон FSM + подключение/отключение сети
TEST_F(AppManagerTest, BasicFSM) {
    // 1. Проверка начального IDLE состояния
    EXPECT_TRUE(appManager->isIdle());
    EXPECT_TRUE(appManager->isOffline());

    // 2. Шпион для проверки сигнала signalMakeConnection
    QSignalSpy spyMakeConnection(appManager.get(),
        &uniter::managers::AppManager::signalMakeConnection);

    // 3. Запуск AppManager - переход в STARTED
    appManager->start_run();

    // 4. Проверка STARTED состояния и сигнала подключения
    EXPECT_TRUE(appManager->isStarted());
    EXPECT_EQ(spyMakeConnection.count(), 1);

    // 5. Шпион для проверки сигнала signalConnected (UI разблокировка)
    QSignalSpy spyConnected(appManager.get(),
        &uniter::managers::AppManager::signalConnected);
    // 5.1. Шпион для проверки отправки сигнала signalFindAuthData
    QSignalSpy spyAuthRequest(appManager.get(),
        &uniter::managers::AppManager::signalFindAuthData);

    // 6. Симуляция успешного подключения сети
    mockNetwork->emitConnected();

    // 7. Проверка CONNECTED состояния AppState и ONLINE NetState
    EXPECT_TRUE(appManager->isConnected());
    EXPECT_TRUE(appManager->isOnline());
    EXPECT_EQ(spyConnected.count(), 1);
    EXPECT_EQ(spyAuthRequest.count(), 1);

    // 8. Шпион для проверки сигнала signalDisconnected (UI блокировка)
    QSignalSpy spyDisconnected(appManager.get(),
        &uniter::managers::AppManager::signalDisconnected);

    // 9. Симуляция разрыва соединения
    mockNetwork->emitDisconnected();

    // 10. Проверка: AppState остается CONNECTED (сессия жива),
    // NetState переходит в OFFLINE
    EXPECT_TRUE(appManager->isConnected());
    EXPECT_TRUE(appManager->isOffline());
    EXPECT_EQ(spyDisconnected.count(), 1);

    // 11. Повторное соединение
    mockNetwork->emitConnected();

    // 12. Проверка: после установления соединения AppState не изменяется
    EXPECT_TRUE(appManager->isConnected());
    EXPECT_TRUE(appManager->isOnline());
    EXPECT_EQ(spyAuthRequest.count(), 2);

    // 13. Разрываем соединение и отправляем authMessage
    mockNetwork->emitDisconnected();
    mockAuthWidget->emitUserData();
    QSignalSpy spySendMessage(appManager.get(),
        &uniter::managers::AppManager::signalSendUniterMessage);

    // 14. Соединяем
    mockNetwork->emitConnected();

    // 15. Проверка состояния и отправки сообщений
    EXPECT_TRUE(appManager->isConnected());
    EXPECT_TRUE(appManager->isOnline());
    EXPECT_EQ(spyAuthRequest.count(), 2);
    EXPECT_EQ(spySendMessage.count(), 1);

}

// Сценарии неудачной аутентификации
TEST_F(AppManagerTest, badAuth) {
    // 0. Шпион передачи сообщений в сеть
    QSignalSpy spyMessagesSend(appManager.get(), &managers::AppManager::signalSendUniterMessage);
    // Шпиен получения аутентификации
    QSignalSpy spyAuthProc(appManager.get(), &managers::AppManager::signalAuthed);

    // 1. Базовый FSM до запроса авторизации
    appManager->start_run();
    mockNetwork->emitConnected();
    mockAuthWidget->emitUserData();

    // 2. Проверка получения запроса
    EXPECT_EQ(spyMessagesSend.count(), 1);

    // 3. Генерируем ответ
    mockNetwork->emitAuthError();

    // 4. Проверяем что обработка произошла
    EXPECT_EQ(spyAuthProc.count(), 1);
    EXPECT_FALSE(mockAuthWidget->authed);

    // 5. Генерируем повторный сценарий
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthError();

    // 6. Повторные проверки
    EXPECT_EQ(spyMessagesSend.count(), 2);
    EXPECT_EQ(spyAuthProc.count(), 2);
    EXPECT_FALSE(mockAuthWidget->authed);
}

// Сценарий удачной аутентификации
TEST_F(AppManagerTest, goodAuth) {
    // 0. Шпион передачи сообщений в сеть
    QSignalSpy spyMessagesSend(appManager.get(), &managers::AppManager::signalSendUniterMessage);

    // 1. Базовый FSM до запроса авторизации
    appManager->start_run();
    mockNetwork->emitConnected();
    mockAuthWidget->emitUserData();

    // 2. Проверка получения запроса сетевым классом
    EXPECT_EQ(spyMessagesSend.count(), 1);

    // Шпиен получения аутентификации
    QSignalSpy spyAuthProc(appManager.get(), &managers::AppManager::signalAuthed);
    // Шпиен запроса конфигурации
    QSignalSpy spyConfigProc(appManager.get(), &managers::AppManager::signalConfigProc);

    // 3. Генерируем ответ (успешная авторизация)
    mockNetwork->emitAuthSuccess();

    // 4. Проверяем что обработка произошла
    EXPECT_EQ(spyAuthProc.count(), 1);
    EXPECT_EQ(spyConfigProc.count(), 1);
    EXPECT_TRUE(mockAuthWidget->authed);
}


// Сценарии конфигурирования и локальных настроек
TEST_F(AppManagerTest, postAuthSettings) {

    // 0. Шпионы за FSM переходами
    QSignalSpy spyConfigProc(appManager.get(), &managers::AppManager::signalConfigProc);
    QSignalSpy spyCustomizeProc(appManager.get(), &managers::AppManager::signalCustomizeProc);
    QSignalSpy spyPollMessages(appManager.get(), &managers::AppManager::signalPollMessages);

    // Шпион пересылки сообщений appmanager вниз (CRUD forwarding)
    QSignalSpy spyRecvMessages(appManager.get(), &managers::AppManager::signalRecvUniterMessage);

    // 1. BasicFSM → AUTHENTIFICATED
    appManager->start_run();
    mockNetwork->emitConnected();
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();

    // 2. Проверяем переход AUTHENTIFICATED
    EXPECT_EQ(spyConfigProc.count(), 1);      // signalConfigProc(User)

    // 3. ConfigManager подтверждает → CONFIGURATED
    mockConfigManager->emitConfigured();
    EXPECT_EQ(spyCustomizeProc.count(), 1);   // signalCustomizeProc()

    // 4. LocalManager подтверждает → READY + POLL
    mockLocalManager->emitCustomized();
    EXPECT_EQ(spyPollMessages.count(), 1);    // signalPollMessages()

    // 5. READY: проверяем CRUD forwarding!
    mockNetwork->emitCrudMessage();
    EXPECT_EQ(spyRecvMessages.count(), 1);    // signalRecvUniterMessage(Purchase)

    // 6. Бонус: проверяем параметры CRUD сообщения
    auto crudArgs = spyRecvMessages.takeFirst();
    auto message = qvariant_cast<std::shared_ptr<messages::UniterMessage>>(crudArgs.at(0));
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->subsystem, messages::Subsystem::PURCHASES);
    EXPECT_EQ(message->crudact, messages::CrudAction::CREATE);
}



} //namespace appmanagertest

#include "appmanagertest.moc"
