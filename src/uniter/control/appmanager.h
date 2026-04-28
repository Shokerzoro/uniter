
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
     * AppManager is the main FSM of the application.
     *
     * Key state semantics (see docs/FSM-AppManager.pdf):
     * — The name of the state reflects the ACTION upon entry, not the event.
     * — Network states are mirrored offline. If the network is lost entry-
     * action is not repeated: NetState is removed and the UI is blocked.
     * When the network returns online - for network states entry-action
     * repeats (to complete the action), for local ones - not.
     * — Local states (DBLOADING, CONFIGURATING, KCONNECTOR, DBCLEAR)
     * network independent: network loss only changes NetState, not
     * interrupts them; the network return does not pull entry again.
     *
     * Enlarged sequence (golden path):
     *   IDLE → STARTED → AUTHENIFICATION → IDLE_AUTHENIFICATION
     *        → DBLOADING → CONFIGURATING → KCONNECTOR → KAFKA
     *        → (DBCLEAR → SYNC →) READY
     * SHUTDOWN - reachable from any state.
     *
     * Entry-actions:
     * STARTED - signalMakeConnection (Network: establish connection)
     * AUTHENIFICATION - signalFindAuthData (AuthWidget) or if data
     * already in the buffer, immediately UniterMessage AUTH REQUEST
     * IDLE_AUTHENIFICATION - waiting for a response from the server (no action)
     * DBLOADING - signalLoadResources(userhash) in DataManager
     * CONFIGURATING - signalConfigProc(User) in ConfigManager
     * KCONNECTOR - signalInitKafkaConnector(userhash) in KafkaConnector
     *   KAFKA             — UniterMessage GET_KAFKA_CREDENTIALS REQUEST
     * (with offset in add_data) to the network
     * DBCLEAR - signalClearDatabase() in DataManager
     * SYNC - UniterMessage FULL_SYNC REQUEST to the network
     *   READY             — signalSubscribeKafka (KafkaConnector)
     * SHUTDOWN - saving settings and message buffers
     *
     * Network states (have an offline mirror, entry is repeated when
     * return online): AUTHENIFICATION, IDLE_AUTHENIFICATION, KAFKA, SYNC,
     * READY.
     * Local states (there is no offline mirror, entry is not repeated):
     * DBLOADING, CONFIGURATING, KCONNECTOR, DBCLEAR.
     */
    class AppManager : public QObject {
        Q_OBJECT

    public:
        // FSM Events
        enum class Events {
            START,                  // Launching the application
            NET_CONNECTED,          // Network connected
            NET_DISCONNECTED,       // Network disabled
            AUTH_DATA_READY,        // Widget sent login/password (AUTHENIFICATION → IDLE_AUTHENIFICATION)
            AUTH_SUCCESS,           // Successful authentication from server
            AUTH_FAIL,              // Failed authentication (remain in AUTHENIFICATION)
            DB_LOADED,              // DB loaded
            CONFIG_DONE,            // Local configuration complete
            OFFSET_RECEIVED,        // KafkaConnector sent offset → KCONNECTOR → KAFKA
            OFFSET_ACTUAL,          // The server confirmed the relevance of offset → READY
            OFFSET_STALE,           // Server: offset is deprecated → DBCLEAR
            DB_CLEARED,             // DataManager confirmed DB cleanup → SYNC
            SYNC_DONE,              // Full-sync completed → READY
            LOGOUT,                 // Deauthorization → IDLE_AUTHENIFICATION
            SHUTDOWN                // Shutdown
        };

        // Application states (public enum - needed for tests/debugging)
        enum class AppState {
            IDLE,                   // offline, starting
            STARTED,                // waiting for connection
            AUTHENIFICATION,        // wait for your login/password to be entered
            IDLE_AUTHENIFICATION,   // We have sent an authorization request, we are waiting for a response
            DBLOADING,              // [local] database initialization
            CONFIGURATING,          // [local] local configuration
            KCONNECTOR,             // [local] initializing KafkaConnector
            KAFKA,                  // checking the server offset is up to date
            DBCLEAR,                // [local] full database cleanup before sync
            SYNC,                   // full-sync with server
            READY,                  // working state (subscription to Kafka)
            SHUTDOWN                // final
        };

        // Network state (orthogonal to AppState)
        enum class NetState {
            ONLINE,
            OFFLINE
        };

    private:
        // Private constructor for singleton
        AppManager();

        // Prohibition of copying and moving
        AppManager(const AppManager&) = delete;
        AppManager& operator=(const AppManager&) = delete;
        AppManager(AppManager&&) = delete;
        AppManager& operator=(AppManager&&) = delete;

        // Current states
        AppState m_appState = AppState::IDLE;
        NetState m_netState = NetState::OFFLINE;

        // Temporary data
        std::shared_ptr<contract::UniterMessage> m_authMessage;
        std::shared_ptr<contract::manager::Employee> m_user;
        QString m_lastKafkaOffset;     // last offset from KafkaConnector

        // Main point of event processing
        void ProcessEvent(Events event);

        // Entry-actions for states (side-effects)
        void enterStarted();
        void enterAuthenification();
        void enterIdleAuthenification();
        void enterDbLoading();
        void enterConfigurating();
        void enterKafkaConnector();   // KCONNECTOR
        void enterKafka();            // KAFKA (offset relevance network query)
        void enterDbClear();          // DBCLEAR
        void enterSync();             // SYNC (network request FULL_SYNC)
        void enterReady();
        void enterShutdown();

        // Re-run entry-action for online states when returning online.
        // For local states - no-op.
        void reenterOnReconnect();

        // Routing of incoming messages (see docs/appmanager_routing.md).
        void handleInitProtocolMessage(std::shared_ptr<contract::UniterMessage> message);
        void handleReadyProtocolMessage(std::shared_ptr<contract::UniterMessage> message);
        void handleReadyCrudMessage(std::shared_ptr<contract::UniterMessage> message);

        // Outgoing message routing based on subsystem/protact.
        void dispatchOutgoing(std::shared_ptr<contract::UniterMessage> message);

        // Auxiliary
        static bool isNetworkDependent(AppState s); // does the state have an offline mirror?
        static bool isInsideUiLockedArea(AppState s);

        // Formation of user hash (now stub: id → ASCII)
        QByteArray makeUserHash() const;

    public:
        // Public static method to get an instance
        static AppManager* instance();

        ~AppManager() override = default;

        void start_run();

        // For testing/debugging
        AppState currentState() const { return m_appState; }
        NetState currentNetState() const { return m_netState; }

        // Resetting the singleton state is only for unit tests.
        // In production, AppManager exists once for the entire lifecycle of the process.
        void resetForTest() {
            m_appState = AppState::IDLE;
            m_netState = NetState::OFFLINE;
            m_authMessage.reset();
            m_user.reset();
            m_lastKafkaOffset.clear();
        }

    public:
        // Helper: type of MinIO action (for tests/diagnostics).
        static bool isMinioProtocolAction(contract::ProtocolAction a);

    public slots:
        // From network class
        void onConnectionUpdated(bool state);

        // From managers / transitions between states
        void onResourcesLoaded();                   // DBLOADING → CONFIGURATING
        void onConfigured();                        // CONFIGURATING → KCONNECTOR
        void onDatabaseCleared();                   // DBCLEAR → SYNC
        void onLogout();                            // READY → IDLE_AUTHENIFICATION
        void onShutDown();

        // Kafka / KafkaConnector
        void onKafkaOffsetReceived(QString offset); // KCONNECTOR → KAFKA

        // Single entry point for incoming UniterMessage
        // from all three network classes (Server/Kafka/Minio → AppManager).
        void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);

        // Single point of entry for outgoing UniterMessage
        // from FileManager and UI widgets (in runtime), as well as from FSM itself
        // (enterAuthenification/enterKafka/enterSync).
        void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message);

    signals:
        // === External (network) ===
        void signalMakeConnection();                 // Network: establish a connection

        // === KafkaConnector ===
        void signalInitKafkaConnector(QByteArray userhash); // KCONNECTOR: init per user
        void signalSubscribeKafka();                        // READY: broadcast subscription

        // === UI ===
        void signalConnectionUpdated(bool state);    // online/offline indication
        void signalAuthed(bool result);              // authentication result
        void signalLoggedOut();                      // logout confirmation

        // === To managers ===
        void signalFindAuthData();                   // AuthWidget: give me your login/password
        void signalLoadResources(QByteArray userhash);
        void signalConfigProc(std::shared_ptr<contract::manager::Employee> User);
        void signalClearResources();                 // logout cleanup (cache/browsers)
        void signalClearDatabase();                  // DBCLEAR: complete clearing of database tables

        // === UniterMessage Routing ===
        // Incoming traffic (onRecvUniterMessage) is decomposed into two signals:
        // signalRecvUniterMessage → DataManager (in READY: CRUD over resources)
        // signalForwardToFileManager → FileManager (in READY: MINIO protocols)
        void signalRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message);
        void signalForwardToFileManager(std::shared_ptr<contract::UniterMessage> Message);

        // Outgoing traffic (onSendUniterMessage) is dispatched into two
        // separate signals connected in main.cpp:
        //   signalSendToServer  → ServerConnector (AUTH, GET_KAFKA_CREDENTIALS,
        // FULL_SYNC, GET_MINIO_PRESIGNED_URL, CRUD in READY)
        //   signalSendToMinio   → MinIOConnector  (GET_MINIO_FILE, PUT_MINIO_FILE)
        void signalSendToServer(std::shared_ptr<contract::UniterMessage> Message);
        void signalSendToMinio(std::shared_ptr<contract::UniterMessage> Message);
    };


} // namespace uniter::control

#endif // APPMANAGER_H
