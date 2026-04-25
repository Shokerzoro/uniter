
#ifndef APPMANAGER_H
#define APPMANAGER_H

#include "../contract/unitermessage.h"
#include "../contract/manager/employee.h"
#include <QObject>
#include <QByteArray>
#include <optional>
#include <memory>

namespace uniter::control {

    /*
     * AppManager — главный FSM приложения.
     *
     * Ключевая семантика состояний (см. docs/FSM-AppManager.pdf):
     *   — Название состояния отражает ДЕЙСТВИЕ при входе, а не событие.
     *   — Сетевые состояния имеют зеркальное offline. При потере сети entry-
     *     action не повторяется: снимается NetState и UI блокируется.
     *     При возврате сети online — для сетевых состояний entry-action
     *     повторяется (чтобы завершить действие), для локальных — нет.
     *   — Локальные состояния (DBLOADING, CONFIGURATING, KCONNECTOR, DBCLEAR)
     *     не зависят от сети: потеря сети меняет только NetState, но не
     *     прерывает их; возврат сети не дёргает entry повторно.
     *
     * Укрупнённая последовательность (golden path):
     *   IDLE → STARTED → AUTHENIFICATION → IDLE_AUTHENIFICATION
     *        → DBLOADING → CONFIGURATING → KCONNECTOR → KAFKA
     *        → (DBCLEAR → SYNC →) READY
     *   SHUTDOWN — достижим из любого состояния.
     *
     * Entry-actions:
     *   STARTED           — signalMakeConnection (Network: установить соединение)
     *   AUTHENIFICATION   — signalFindAuthData (AuthWidget) либо, если данные
     *                       уже в буфере, сразу UniterMessage AUTH REQUEST
     *   IDLE_AUTHENIFICATION — ждём ответ от сервера (нет действий)
     *   DBLOADING         — signalLoadResources(userhash) в DataManager
     *   CONFIGURATING     — signalConfigProc(User) в ConfigManager
     *   KCONNECTOR        — signalInitKafkaConnector(userhash) в KafkaConnector
     *   KAFKA             — UniterMessage GET_KAFKA_CREDENTIALS REQUEST
     *                       (с offset в add_data) в сеть
     *   DBCLEAR           — signalClearDatabase() в DataManager
     *   SYNC              — UniterMessage FULL_SYNC REQUEST в сеть
     *   READY             — signalSubscribeKafka (KafkaConnector)
     *   SHUTDOWN          — сохранение настроек и буферов сообщений
     *
     * Сетевые состояния (имеют offline-зеркало, entry повторяется при
     * возврате online): AUTHENIFICATION, IDLE_AUTHENIFICATION, KAFKA, SYNC,
     * READY.
     * Локальные состояния (offline-зеркала нет, entry не повторяется):
     * DBLOADING, CONFIGURATING, KCONNECTOR, DBCLEAR.
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
            OFFSET_RECEIVED,        // KafkaConnector прислал offset → KCONNECTOR → KAFKA
            OFFSET_ACTUAL,          // Сервер подтвердил актуальность offset → READY
            OFFSET_STALE,           // Сервер: offset устарел → DBCLEAR
            DB_CLEARED,             // DataManager подтвердил очистку БД → SYNC
            SYNC_DONE,              // Full-sync завершён → READY
            LOGOUT,                 // Деавторизация → IDLE_AUTHENIFICATION
            SHUTDOWN                // Завершение работы
        };

        // Состояния приложения (публичный enum — нужен тестам/отладке)
        enum class AppState {
            IDLE,                   // offline, стартовое
            STARTED,                // ждём соединения
            AUTHENIFICATION,        // ждём ввод логина/пароля
            IDLE_AUTHENIFICATION,   // отправили запрос авторизации, ждём ответа
            DBLOADING,              // [local] инициализация БД
            CONFIGURATING,          // [local] локальная конфигурация
            KCONNECTOR,             // [local] инициализация KafkaConnector
            KAFKA,                  // проверка актуальности offset у сервера
            DBCLEAR,                // [local] полная очистка БД перед sync
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
        std::shared_ptr<contract::manager::Employee> m_user;
        QString m_lastKafkaOffset;     // последний offset от KafkaConnector

        // Основная точка обработки события
        void ProcessEvent(Events event);

        // Entry-actions для состояний (side-effects)
        void enterStarted();
        void enterAuthenification();
        void enterIdleAuthenification();
        void enterDbLoading();
        void enterConfigurating();
        void enterKafkaConnector();   // KCONNECTOR
        void enterKafka();            // KAFKA (сетевой запрос актуальности offset)
        void enterDbClear();          // DBCLEAR
        void enterSync();             // SYNC (сетевой запрос FULL_SYNC)
        void enterReady();
        void enterShutdown();

        // Повторный запуск entry-action для сетевых состояний при возврате online.
        // Для локальных состояний — no-op.
        void reenterOnReconnect();

        // Маршрутизация входящих сообщений (см. docs/appmanager_routing.md).
        void handleInitProtocolMessage(std::shared_ptr<contract::UniterMessage> message);
        void handleReadyProtocolMessage(std::shared_ptr<contract::UniterMessage> message);
        void handleReadyCrudMessage(std::shared_ptr<contract::UniterMessage> message);

        // Маршрутизация исходящих сообщений на основе subsystem/protact.
        void dispatchOutgoing(std::shared_ptr<contract::UniterMessage> message);

        // Вспомогательное
        static bool isNetworkDependent(AppState s); // имеет ли состояние offline-зеркало
        static bool isInsideUiLockedArea(AppState s);

        // Формирование hash пользователя (сейчас stub: id → ASCII)
        QByteArray makeUserHash() const;

    public:
        // Публичный статический метод получения экземпляра
        static AppManager* instance();

        ~AppManager() override = default;

        void start_run();

        // Для тестирования/отладки
        AppState currentState() const { return m_appState; }
        NetState currentNetState() const { return m_netState; }

        // Сброс состояния синглтона — только для unit-тестов.
        // В продакшене AppManager существует один раз на весь lifecycle процесса.
        void resetForTest() {
            m_appState = AppState::IDLE;
            m_netState = NetState::OFFLINE;
            m_authMessage.reset();
            m_user.reset();
            m_lastKafkaOffset.clear();
        }

    public:
        // Helper: тип MinIO-действия (для тестов/диагностики).
        static bool isMinioProtocolAction(contract::ProtocolAction a);

    public slots:
        // От сетевого класса
        void onConnectionUpdated(bool state);

        // От менеджеров / переходы между состояниями
        void onResourcesLoaded();                   // DBLOADING → CONFIGURATING
        void onConfigured();                        // CONFIGURATING → KCONNECTOR
        void onDatabaseCleared();                   // DBCLEAR → SYNC
        void onLogout();                            // READY → IDLE_AUTHENIFICATION
        void onShutDown();

        // Kafka / KafkaConnector
        void onKafkaOffsetReceived(QString offset); // KCONNECTOR → KAFKA

        // Единая точка входа входящих UniterMessage
        // от всех трёх сетевых классов (Server/Kafka/Minio → AppManager).
        void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);

        // Единая точка входа исходящих UniterMessage
        // от FileManager и UI-виджетов (в рантайме), а также от самой FSM
        // (enterAuthenification/enterKafka/enterSync).
        void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);

    signals:
        // === Внешние (сеть) ===
        void signalMakeConnection();                 // Network: установить соединение

        // === KafkaConnector ===
        void signalInitKafkaConnector(QByteArray userhash); // KCONNECTOR: init под пользователя
        void signalSubscribeKafka();                        // READY: подписка на broadcast

        // === UI ===
        void signalConnectionUpdated(bool state);    // online/offline индикация
        void signalAuthed(bool result);              // результат аутентификации
        void signalLoggedOut();                      // подтверждение логаута

        // === К менеджерам ===
        void signalFindAuthData();                   // AuthWidget: дай логин/пароль
        void signalLoadResources(QByteArray userhash);
        void signalConfigProc(std::shared_ptr<contract::manager::Employee> User);
        void signalClearResources();                 // logout-очистка (кеш/обзерверы)
        void signalClearDatabase();                  // DBCLEAR: полная очистка таблиц БД

        // === Маршрутизация UniterMessage ===
        // Входящий трафик (onRecvUniterMessage) раскладывается по двум сигналам:
        //   signalRecvUniterMessage     → DataManager (в READY: CRUD над ресурсами)
        //   signalForwardToFileManager  → FileManager (в READY: MINIO-протоколизмы)
        void signalRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
        void signalForwardToFileManager(std::shared_ptr<contract::UniterMessage> Message);

        // Исходящий трафик (onSendUniterMessage) диспетчеризуется на два
        // отдельных сигнала, подключаемых в main.cpp:
        //   signalSendToServer  → ServerConnector (AUTH, GET_KAFKA_CREDENTIALS,
        //                          FULL_SYNC, GET_MINIO_PRESIGNED_URL, CRUD в READY)
        //   signalSendToMinio   → MinIOConnector  (GET_MINIO_FILE, PUT_MINIO_FILE)
        void signalSendToServer(std::shared_ptr<contract::UniterMessage> Message);
        void signalSendToMinio(std::shared_ptr<contract::UniterMessage> Message);
    };


} // namespace uniter::control

#endif // APPMANAGER_H
