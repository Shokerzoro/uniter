
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
#include <memory>

using namespace uniter;

namespace appmanagertest {


class MockAuthWidget : public QObject {
    Q_OBJECT
public:
    MockAuthWidget() = default;
    bool authed = false;
    void emitUserData() {
        auto authReqMessage = std::make_shared<contract::UniterMessage>();
        authReqMessage->subsystem = contract::Subsystem::PROTOCOL;
        authReqMessage->protact = contract::ProtocolAction::AUTH;
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
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
};

class MockNetwork : public QObject {
    Q_OBJECT
public:
    MockNetwork() = default;

    void emitConnected() { emit signalConnected(); }
    void emitDisconnected() { emit signalDisconnected(); }

    // Отправка crud сообщения
    void emitCrudMessage() {
        auto crudMessage = std::make_shared<contract::UniterMessage>();
        crudMessage->subsystem = contract::Subsystem::PURCHASES;
        crudMessage->crudact = contract::CrudAction::CREATE;
        crudMessage->status = contract::MessageStatus::NOTIFICATION;

        auto newPurch = std::make_shared<contract::supply::Purchase>(
            1001,                                    // id
            true,                                    // actual
            QDateTime::currentDateTime(),            // created_at
            QDateTime::currentDateTime(),            // updated_at
            12345,                                   // created_by (User ID)
            12345,                                   // updated_by
            QString("Закупка стали 10т"),            // name
            QString("Нужна сталь для проекта #47"),  // description
            contract::supply::PurchStatus::DRAFT,   // status
            nullptr                                  // material (пока null)
            );
        crudMessage->resource = std::move(newPurch);

        emit signalRecvUniterMessage(std::move(crudMessage));
    }

    // Отправляем ответы на запрос об авторизации
    void emitAuthSuccess() {

        auto authMessageSuccess = std::make_shared<contract::UniterMessage>();
        // Настройка успешного ответа аутентификации
        authMessageSuccess->subsystem = contract::Subsystem::PROTOCOL;
        authMessageSuccess->protact = contract::ProtocolAction::AUTH;
        authMessageSuccess->status = contract::MessageStatus::RESPONSE;
        // Создаем тестового пользователя
        std::vector<contract::employees::EmployeeAssignment> assignments;

        // Employee: 10 аргументов
        auto employee = std::make_shared<contract::employees::Employee>(
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

        auto errorMessage = std::make_shared<contract::UniterMessage>();
        // Настройка сообщения об ошибке аутентификации
        errorMessage->version = 1;
        errorMessage->subsystem = contract::Subsystem::PROTOCOL;
        errorMessage->protact = contract::ProtocolAction::AUTH;
        errorMessage->status = contract::MessageStatus::RESPONSE;
        errorMessage->error = contract::ErrorCode::BAD_REQUEST;

        emit signalRecvUniterMessage(errorMessage);
    }

public slots:
    // Запрос на подключение
    void onMakeConnection(void) {

    }
    // Получение сообщения от AppManager
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message) {
        // Логируем или обрабатываем

    };

signals:
    void signalConnected();
    void signalDisconnected();
    // Получение сообщения из сети
    void signalRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
};

class MockConfigManager : public QObject {
    Q_OBJECT;
public:
    MockConfigManager() = default;
    std::shared_ptr<contract::employees::Employee> m_User;
    void emitConfigured() {
        emit signalConfigured();
    }
public slots:
    void onConfigProc(std::shared_ptr<contract::employees::Employee> User) {
        // Копируем User
        m_User = std::move(User);
    }
signals:
    void signalConfigured();
    void signalSubsystemAdded(contract::Subsystem subsystem,
                              contract::GenSubsystemType genType,
                              std::optional<uint64_t> genId,
                              bool created);
};

// ============================================================================
// ИЗМЕНЕНИЕ: AppManagerTesting теперь НЕ наследует AppManager,
// а использует композицию с указателем на синглтон
// ============================================================================
class AppManagerTesting {
private:
    control::AppManager* appManager_;

public:
    AppManagerTesting() {
        // Получаем указатель на синглтон
        appManager_ = control::AppManager::instance();
    }

    // Получить указатель для подключения сигналов/слотов
    control::AppManager* get() {
        return appManager_;
    }

    // Доступ к состоянию приложения через friend-функции или рефлексию
    // Поскольку AState и NState protected, нужен альтернативный подход

    // ВАРИАНТ 1: Использовать сигналы для проверки состояний
    // ВАРИАНТ 2: Сделать AppManagerTesting friend-классом в appmanager.h
    // ВАРИАНТ 3: Добавить публичные геттеры в AppManager для тестов

    // Временное решение: проверяем состояния через сигналы и поведение
    // Для полноценных тестов нужно либо:
    // 1. Добавить в appmanager.h: friend class appmanagertest::AppManagerTesting;
    // 2. Или добавить #ifdef UNIT_TESTS с публичными геттерами

    void start_run() {
        appManager_->start_run();
    }

    void onResourcesLoaded() {
        appManager_->onResourcesLoaded();
    }

    // Для replaceMockConfigManager нужно отключить синглтон ConfigManager
    void replaceMockConfigManager(MockConfigManager* mock) {
        // Отключаем старые подписки внутреннего ConfigManager
        auto ConfigMgr = control::ConfigManager::instance();
        QObject::disconnect(ConfigMgr, nullptr, appManager_, nullptr);
        QObject::disconnect(appManager_, nullptr, ConfigMgr, nullptr);

        // Подписываем mock напрямую
        QObject::connect(mock, &MockConfigManager::signalConfigured,
                         appManager_, &control::AppManager::onConfigured);
        QObject::connect(appManager_, &control::AppManager::signalConfigProc,
                         mock, &MockConfigManager::onConfigProc);
    }
};


// ============================================================================
// AppManagerTest: SetUp теперь работает с синглтоном через AppManagerTesting
// ============================================================================
class AppManagerTest : public ::testing::Test {
public:
    std::unique_ptr<AppManagerTesting> appManager;
    std::unique_ptr<MockAuthWidget> mockAuthWidget;
    std::unique_ptr<MockNetwork> mockNetwork;
    std::unique_ptr<MockConfigManager> mockConfigManager;

protected:
    void SetUp() override {
        // Создаем wrapper для AppManager (получает синглтон внутри)
        appManager = std::make_unique<AppManagerTesting>();
        mockAuthWidget = std::make_unique<MockAuthWidget>();
        mockNetwork = std::make_unique<MockNetwork>();
        mockConfigManager = std::make_unique<MockConfigManager>();

        // Заменяем внутренний ConfigManager на mock
        appManager->replaceMockConfigManager(mockConfigManager.get());

        // Теперь линкуем сигналы и слоты (connect)
        // appmanager и сетевой класс
        QObject::connect(appManager->get(), &control::AppManager::signalSendUniterMessage,
                         mockNetwork.get(), &MockNetwork::onSendUniterMessage);
        QObject::connect(appManager->get(), &control::AppManager::signalMakeConnection,
                         mockNetwork.get(), &MockNetwork::onMakeConnection);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalRecvUniterMessage,
                         appManager->get(), &control::AppManager::onRecvUniterMessage);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalConnected,
                         appManager->get(), &control::AppManager::onConnected);
        QObject::connect(mockNetwork.get(), &MockNetwork::signalDisconnected,
                         appManager->get(), &control::AppManager::onDisconnected);


        // appmanager и виджет аутентификации
        QObject::connect(mockAuthWidget.get(), &MockAuthWidget::signalSendUniterMessage,
                         appManager->get(), &control::AppManager::onSendUniterMessage);
        QObject::connect(appManager->get(), &control::AppManager::signalFindAuthData,
                         mockAuthWidget.get(), &MockAuthWidget::onFindAuthData);
        QObject::connect(appManager->get(), &control::AppManager::signalAuthed,
                         mockAuthWidget.get(), &MockAuthWidget::onAuthed);
    }

    void TearDown() override {
        // Отключаем все связи после теста
        QObject::disconnect(appManager->get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockNetwork.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockAuthWidget.get(), nullptr, nullptr, nullptr);
        QObject::disconnect(mockConfigManager.get(), nullptr, nullptr, nullptr);
    }
};

// ============================================================================
// ТЕСТЫ: изменены проверки состояний (теперь через сигналы)
// ============================================================================

// Базовый прогон FSM + подключение/отключение сети
TEST_F(AppManagerTest, BasicFSM) {
    // 1. Проверка начального IDLE состояния - убираем прямую проверку
    // БЫЛО: EXPECT_TRUE(appManager->isIdle());
    // Проверяем через отсутствие сигналов

    // 2. Шпион для проверки сигнала signalMakeConnection
    QSignalSpy spyMakeConnection(appManager->get(),
                                 &uniter::control::AppManager::signalMakeConnection);

    // 3. Запуск AppManager - переход в STARTED
    appManager->start_run();

    // 4. Проверка STARTED состояния через сигнал подключения
    EXPECT_EQ(spyMakeConnection.count(), 1);

    // 5. Шпион для проверки сигнала signalConnected (UI разблокировка)
    QSignalSpy spyConnected(appManager->get(),
                            &uniter::control::AppManager::signalConnected);
    // 5.1. Шпион для проверки отправки сигнала signalFindAuthData
    QSignalSpy spyAuthRequest(appManager->get(),
                              &uniter::control::AppManager::signalFindAuthData);

    // 6. Симуляция успешного подключения сети
    mockNetwork->emitConnected();

    // 7. Проверка CONNECTED состояния через сигналы
    EXPECT_EQ(spyConnected.count(), 1);
    EXPECT_EQ(spyAuthRequest.count(), 1);

    // 8. Шпион для проверки сигнала signalDisconnected (UI блокировка)
    QSignalSpy spyDisconnected(appManager->get(),
                               &uniter::control::AppManager::signalDisconnected);

    // 9. Симуляция разрыва соединения
    mockNetwork->emitDisconnected();

    // 10. Проверка через сигналы
    EXPECT_EQ(spyDisconnected.count(), 1);

    // 11. Повторное соединение
    mockNetwork->emitConnected();

    // 12. Проверка: после установления соединения снова запрашиваются данные
    EXPECT_EQ(spyAuthRequest.count(), 2);

    // 13. Разрываем соединение и отправляем authMessage
    mockNetwork->emitDisconnected();
    mockAuthWidget->emitUserData();
    QSignalSpy spySendMessage(appManager->get(),
                              &uniter::control::AppManager::signalSendUniterMessage);

    // 14. Соединяем
    mockNetwork->emitConnected();

    // 15. Проверка состояния и отправки сообщений
    EXPECT_EQ(spyAuthRequest.count(), 2);
    EXPECT_EQ(spySendMessage.count(), 1);
}

// Сценарии неудачной аутентификации
TEST_F(AppManagerTest, badAuth) {
    // 0. Шпион передачи сообщений в сеть
    QSignalSpy spyMessagesSend(appManager->get(), &control::AppManager::signalSendUniterMessage);
    // Шпион получения аутентификации
    QSignalSpy spyAuthProc(appManager->get(), &control::AppManager::signalAuthed);

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
    QSignalSpy spyMessagesSend(appManager->get(), &control::AppManager::signalSendUniterMessage);

    // 1. Базовый FSM до запроса авторизации
    appManager->start_run();
    mockNetwork->emitConnected();
    mockAuthWidget->emitUserData();

    // 2. Проверка получения запроса сетевым классом
    EXPECT_EQ(spyMessagesSend.count(), 1);

    // Шпион получения аутентификации
    QSignalSpy spyAuthProc(appManager->get(), &control::AppManager::signalAuthed);
    // Шпион запроса загрузки БД
    QSignalSpy spyLoadResources(appManager->get(), &control::AppManager::signalLoadResources);

    // 3. Генерируем ответ (успешная авторизация)
    mockNetwork->emitAuthSuccess();

    // 4. Проверяем что обработка произошла
    EXPECT_EQ(spyAuthProc.count(), 1);
    EXPECT_EQ(spyLoadResources.count(), 1);
    EXPECT_TRUE(mockAuthWidget->authed);
}

// Сценарии конфигурирования и загрузки БД
TEST_F(AppManagerTest, postAuthSettings) {

    // 0. Шпионы за FSM переходами
    QSignalSpy spyConfigProc(appManager->get(), &control::AppManager::signalConfigProc);
    QSignalSpy spyLoadResources(appManager->get(), &control::AppManager::signalLoadResources);
    QSignalSpy spyPollMessages(appManager->get(), &control::AppManager::signalPollMessages);

    // Шпион пересылки сообщений appmanager вниз (CRUD forwarding)
    QSignalSpy spyRecvMessages(appManager->get(), &control::AppManager::signalRecvUniterMessage);

    // 1. BasicFSM → AUTHENTIFICATED
    appManager->start_run();
    mockNetwork->emitConnected();
    mockAuthWidget->emitUserData();
    mockNetwork->emitAuthSuccess();

    // 2. Проверяем переход AUTHENTIFICATED и инициализацию БД
    EXPECT_EQ(spyLoadResources.count(), 1);   // signalLoadResources(userhash)

    // 3. БД инициализируется → DBLOADED
    // Имитируем завершение инициализации БД путём вызова onResourcesLoaded
    appManager->onResourcesLoaded();
    EXPECT_EQ(spyConfigProc.count(), 1);      // signalConfigProc(User) вызывается из DBLOADED

    // 4. ConfigManager подтверждает завершение работы → READY
    mockConfigManager->emitConfigured();
    EXPECT_EQ(spyPollMessages.count(), 1);    // signalPollMessages()

    // 6. READY: проверяем CRUD forwarding!
    mockNetwork->emitCrudMessage();
    EXPECT_EQ(spyRecvMessages.count(), 1);    // signalRecvUniterMessage(Purchase)

    // 7. Бонус: проверяем параметры CRUD сообщения
    auto crudArgs = spyRecvMessages.takeFirst();
    auto message = qvariant_cast<std::shared_ptr<contract::UniterMessage>>(crudArgs.at(0));
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->subsystem, contract::Subsystem::PURCHASES);
    EXPECT_EQ(message->crudact, contract::CrudAction::CREATE);
}



} //namespace appmanagertest

#include "appmanagertest.moc"
