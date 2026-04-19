
#ifndef APPMANAGER_H
#define APPMANAGER_H

#include "../contract/unitermessage.h"
#include "../contract/employee/employee.h"
#include <QObject>
#include <QByteArray>
#include <optional>
#include <memory>

namespace uniter::control {

    /*
     * AppManager — главный FSM приложения.
     *
     * Ключевая семантика состояний (см. docs/FSM-AppManager.pdf):
     *   — Названия состояний отражают ДЕЙСТВИЕ, которое выполняется при входе
     *     в состояние, а не событие, приведшее к переходу.
     *   — Каждое online-состояние имеет зеркальное offline (блокировка UI).
     *     Переход online↔offline инициируется сигналом Connected/Disconnected
     *     от сетевого класса и не меняет «логическое» состояние FSM — только
     *     признак доступности сети.
     *
     * Укрупнённая последовательность:
     *   IDLE → STARTED → AUTHENIFICATION → (IDLE_AUTHENIFICATION)
     *        → DBLOADING → CONFIGURATING → KAFKA → (SYNC) → READY
     *   любое состояние (кроме IDLE) → SHUTDOWN
     *
     * Что делает каждое состояние (entry-action):
     *   STARTED           — инициировать установку соединения (MockNetManager::onMakeConnection)
     *   AUTHENIFICATION   — дёрнуть AuthWidget чтобы тот вернул логин/пароль,
     *                       либо сразу отправить сохранённый authMessage
     *   IDLE_AUTHENIFICATION — ждём ответа сервера; никаких действий
     *   DBLOADING         — DataManager::onStartLoadResources(userhash)
     *   CONFIGURATING     — ConfigManager::onConfigProc(User)
     *   KAFKA             — MinIOConnector::onInitConnection() (запросит offset)
     *                       → в ответ придёт offset, который мы переправляем
     *                         серверу для проверки актуальности
     *   SYNC              — signalPollMessages() (запрос FULL_SYNC у сервера)
     *   READY             — подписка на Kafka (signalSubscribeKafka)
     *   SHUTDOWN          — сохранение настроек и буферов сообщений
     */
    class AppManager : public QObject {
        Q_OBJECT

    public:
        // События FSM
        enum class Events {
            START,                  // Запуск приложения
            NET_CONNECTED,          // Сеть подключена
            NET_DISCONNECTED,       // Сеть отключена
            AUTH_DATA_READY,        // Виджет прислал логин/пароль (AUTHENIFICATION → IDLE_AUTHENIFICATION)
            AUTH_SUCCESS,           // Успешная аутентификация от сервера
            AUTH_FAIL,              // Неудачная аутентификация (остаёмся в AUTHENIFICATION)
            DB_LOADED,              // БД загружена
            CONFIG_DONE,            // Локальная конфигурация завершена
            OFFSET_RECEIVED,        // MinIOConnector прислал offset — шлём серверу
            OFFSET_ACTUAL,          // Сервер подтвердил актуальность offset → READY
            OFFSET_STALE,           // Сервер сообщил, что offset устарел → SYNC
            SYNC_DONE,              // Full-sync завершён → READY
            LOGOUT,                 // Деавторизация → IDLE_AUTHENIFICATION
            SHUTDOWN                // Завершение работы
        };

        // Состояния приложения (публичный enum — нужен тестам/отладке)
        enum class AppState {
            IDLE,                   // offline, стартовое
            STARTED,                // offline, ждём соединения
            AUTHENIFICATION,        // ждём ввод логина/пароля
            IDLE_AUTHENIFICATION,   // отправили запрос авторизации, ждём ответа
            DBLOADING,              // инициализируем БД
            CONFIGURATING,          // локальная конфигурация
            KAFKA,                  // запрос offset'а и проверка актуальности
            SYNC,                   // full-sync с сервером
            READY,                  // рабочее состояние (подписка на Kafka)
            SHUTDOWN                // финальное
        };

        // Состояние сети (ортогональное AppState)
        enum class NetState {
            ONLINE,
            OFFLINE
        };

    private:
        // Приватный конструктор для синглтона
        AppManager();

        // Запрет копирования и перемещения
        AppManager(const AppManager&) = delete;
        AppManager& operator=(const AppManager&) = delete;
        AppManager(AppManager&&) = delete;
        AppManager& operator=(AppManager&&) = delete;

        // Текущие состояния
        AppState m_appState = AppState::IDLE;
        NetState m_netState = NetState::OFFLINE;

        // Временные данные
        std::shared_ptr<contract::UniterMessage> m_authMessage;
        std::shared_ptr<contract::employees::Employee> m_user;
        QString m_lastKafkaOffset;     // последний полученный offset от MinIOConnector

        // Основная точка обработки события
        void ProcessEvent(Events event);

        // Entry-actions для состояний (side-effects)
        void enterStarted();
        void enterAuthenification();
        void enterIdleAuthenification();
        void enterDbLoading();
        void enterConfigurating();
        void enterKafka();
        void enterSync();
        void enterReady();
        void enterShutdown();

        // Маршрутизация входящих сообщений
        void handleInitProtocolMessage(std::shared_ptr<contract::UniterMessage> message);
        void handleReadyProtocolMessage(std::shared_ptr<contract::UniterMessage> message);
        void handleReadyCrudMessage(std::shared_ptr<contract::UniterMessage> message);

        // Вспомогательное: проверка, «внутри ли мы UI-блокируемой части FSM»
        static bool isInsideUiLockedArea(AppState s);

    public:
        // Публичный статический метод получения экземпляра
        static AppManager* instance();

        ~AppManager() override = default;

        void start_run();

        // Для тестирования/отладки
        AppState currentState() const { return m_appState; }
        NetState currentNetState() const { return m_netState; }

    public slots:
        // От сетевого класса
        void onConnectionUpdated(bool state);

        // От менеджеров / переходы между состояниями
        void onResourcesLoaded();                   // DBLOADING → CONFIGURATING
        void onConfigured();                        // CONFIGURATING → KAFKA
        void onLogout();                            // READY → IDLE_AUTHENIFICATION
        void onShutDown();

        // Kafka / MinIOConnector
        void onKafkaOffsetReceived(QString offset); // KAFKA: offset от MinIOConnector

        // Маршрутизация сообщений
        void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
        void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);

    signals:
        // === Внешние (сеть) ===
        void signalMakeConnection();                 // Сетевой класс: установить соединение
        void signalPollMessages();                   // Сетевой класс: запросить full-sync

        // === MinIO / Kafka ===
        void signalInitMinIO();                      // MinIOConnector: проинициализироваться, прислать offset
        void signalSubscribeKafka();                 // MinIOConnector: подписаться на broadcast

        // === UI ===
        void signalConnectionUpdated(bool state);    // online/offline индикация
        void signalAuthed(bool result);              // результат аутентификации
        void signalLoggedOut();                      // подтверждение логаута

        // === К менеджерам ===
        void signalFindAuthData();                   // AuthWidget: дай логин/пароль
        void signalLoadResources(QByteArray userhash);
        void signalConfigProc(std::shared_ptr<contract::employees::Employee> User);
        void signalClearResources();

        // === Маршрутизация UniterMessage ===
        void signalRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
        void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
    };


} // namespace uniter::control

#endif // APPMANAGER_H
