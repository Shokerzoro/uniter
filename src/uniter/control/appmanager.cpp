
#include "appmanager.h"
#include "../contract_qt/qt_compat.h"
#include "configmanager.h"
#include "../contract/unitermessage.h"
#include "../contract/manager/employee.h"
#include "../contract_qt/qt_compat.h"

#include <QCryptographicHash>
#include <QDebug>
#include <memory>

namespace uniter::control {

    AppManager::AppManager()
        : QObject(nullptr)
    {
    }

    AppManager* AppManager::instance()
    {
        static AppManager instance;
        return &instance;
    }

    void AppManager::start_run()
    {
        qDebug() << "AppManager::start_run()";
        ProcessEvent(Events::START);
    }

    // ========== ENTRY ACTIONS ==========

    void AppManager::enterStarted()
    {
        qDebug() << "AppManager: enter STARTED — requesting network connection";
        emit signalMakeConnection();
    }

    void AppManager::enterAuthenification()
    {
        qDebug() << "AppManager: enter AUTHENIFICATION — requesting auth data from widget";
        // If the data from the widget is already in the buffer, we send it immediately.
        if (m_authMessage) {
            qDebug() << "AppManager: AUTHENIFICATION — stored authMessage present, sending";
            emit signalSendToServer(m_authMessage);
            // Go to IDLE_AUTHENIFICATION: wait for the server response
            ProcessEvent(Events::AUTH_DATA_READY);
            return;
        }
        // Otherwise, we ask the widget to send its login/password
        emit signalFindAuthData();
    }

    void AppManager::enterIdleAuthenification()
    {
        qDebug() << "AppManager: enter IDLE_AUTHENIFICATION — waiting server response";
        // There are no login actions: we just wait for the server’s response.
    }

    void AppManager::enterDbLoading()
    {
        qDebug() << "AppManager: enter DBLOADING — initializing database";
        emit signalAuthed(true);
        if (m_user) {
            emit signalLoadResources(makeUserHash());
        } else {
            qDebug() << "AppManager: DBLOADING entered without m_user (unexpected)";
        }
    }

    void AppManager::enterConfigurating()
    {
        qDebug() << "AppManager: enter CONFIGURATING — invoking ConfigManager";
        emit signalConfigProc(m_user);
    }

    void AppManager::enterKafkaConnector()
    {
        qDebug() << "AppManager: enter KCONNECTOR — init KafkaConnector";
        // KafkaConnector is initialized for the user, after which
        // will send the saved offset via signalOffsetReady → onKafkaOffsetReceived.
        if (!m_user) {
            qDebug() << "AppManager: KCONNECTOR entered without m_user (unexpected)";
            return;
        }
        emit signalInitKafkaConnector(makeUserHash());
    }

    void AppManager::enterKafka()
    {
        qDebug() << "AppManager: enter KAFKA — requesting server offset actuality check";
        // Network request: GET_KAFKA_CREDENTIALS REQUEST with offset in add_data.
        // In response, the server will send RESPONSE with add_data["offset_actual"] = "true"|"false".
        auto msg = std::make_shared<contract::UniterMessage>();
        msg->subsystem = contract::Subsystem::PROTOCOL;
        msg->protact   = contract::ProtocolAction::GET_KAFKA_CREDENTIALS;
        msg->status    = contract::MessageStatus::REQUEST;
        msg->add_data.emplace("offset", m_lastKafkaOffset.toStdString());
        emit signalSendToServer(msg);
    }

    void AppManager::enterDbClear()
    {
        qDebug() << "AppManager: enter DBCLEAR — clearing database";
        // Local operation: ask the DataManager to clear all database tables.
        // DataManager will respond with signalResourcesCleared -> onDatabaseCleared.
        emit signalClearDatabase();
    }

    void AppManager::enterSync()
    {
        qDebug() << "AppManager: enter SYNC — requesting FULL_SYNC from server";
        // Network request: FULL_SYNC REQUEST (no parameters in add_data).
        // In response, the server will send a CRUD CREATE stream via Kafka, and upon completion -
        // FULL_SYNC SUCCESS in PROTOCOL.
        auto msg = std::make_shared<contract::UniterMessage>();
        msg->subsystem = contract::Subsystem::PROTOCOL;
        msg->protact   = contract::ProtocolAction::FULL_SYNC;
        msg->status    = contract::MessageStatus::REQUEST;
        emit signalSendToServer(msg);
    }

    void AppManager::enterReady()
    {
        qDebug() << "AppManager: enter READY — subscribing to Kafka broadcast";
        emit signalSubscribeKafka();
    }

    void AppManager::enterShutdown()
    {
        qDebug() << "AppManager: enter SHUTDOWN — saving settings & message buffers";
        // TODO: actual saving of settings and message buffers.
    }

    void AppManager::reenterOnReconnect()
    {
        // Called on NET_CONNECTED if the AppState is already inside a UI-locked one
        // areas and status DEPENDS on the network.
        // For local states we do nothing (when the network is returned they
        // just keep executing).
        switch (m_appState) {
            case AppState::AUTHENIFICATION:
                enterAuthenification();
                break;
            case AppState::IDLE_AUTHENIFICATION:
                enterIdleAuthenification();
                break;
            case AppState::KAFKA:
                enterKafka();
                break;
            case AppState::SYNC:
                enterSync();
                break;
            case AppState::READY:
                enterReady();
                break;
            default:
                // IDLE, STARTED, SHUTDOWN, DBLOADING, CONFIGURATING, KCONNECTOR,
                // DBCLEAR - do not restart.
                break;
        }
    }

    bool AppManager::isNetworkDependent(AppState s)
    {
        // States that have an offline mirror on the diagram.
        switch (s) {
            case AppState::AUTHENIFICATION:
            case AppState::IDLE_AUTHENIFICATION:
            case AppState::KAFKA:
            case AppState::SYNC:
            case AppState::READY:
                return true;
            default:
                return false;
        }
    }

    bool AppManager::isInsideUiLockedArea(AppState s)
    {
        // "UI-blockable part" (dotted box in diagram):
        // all states from AUTHENIFICATION to READY inclusive.
        switch (s) {
            case AppState::AUTHENIFICATION:
            case AppState::IDLE_AUTHENIFICATION:
            case AppState::DBLOADING:
            case AppState::CONFIGURATING:
            case AppState::KCONNECTOR:
            case AppState::KAFKA:
            case AppState::DBCLEAR:
            case AppState::SYNC:
            case AppState::READY:
                return true;
            default:
                return false;
        }
    }

    QByteArray AppManager::makeUserHash() const
    {
        // Stub: use the user id as a string.
        // In the future there will be a real hash here (employee id + salt).
        if (!m_user) return {};
        return QByteArray::number(static_cast<qulonglong>(m_user->id));
    }

    // ========== FSM ==========

    void AppManager::ProcessEvent(Events event)
    {
        qDebug() << "AppManager::ProcessEvent() state=" << static_cast<int>(m_appState)
                 << " event=" << static_cast<int>(event);

        // ---------- Handling SHUTDOWN from any state ----------
        if (event == Events::SHUTDOWN) {
            m_appState = AppState::SHUTDOWN;
            enterShutdown();
            return;
        }

        // ---------- Network event processing (online/offline) ----------
        // Principle: NetState is orthogonal to AppState. Loss/return of network not
        // changes AppState (except STARTED → AUTHENIFICATION on the first connection).
        // For online states, when returning online, the entry-action is REPEATED;
        // for local ones - no.
        if (event == Events::NET_DISCONNECTED) {
            if (m_netState == NetState::ONLINE) {
                m_netState = NetState::OFFLINE;
                emit signalConnectionUpdated(false);
            }
            return;
        }
        if (event == Events::NET_CONNECTED) {
            bool was_offline = (m_netState == NetState::OFFLINE);
            m_netState = NetState::ONLINE;
            if (was_offline) {
                emit signalConnectionUpdated(true);
            }
            // STARTED → AUTHENIFICATION upon first connection.
            if (m_appState == AppState::STARTED) {
                m_appState = AppState::AUTHENIFICATION;
                enterAuthenification();
                return;
            }
            // For network states, repeat entry (continue the started action).
            if (was_offline && isNetworkDependent(m_appState)) {
                reenterOnReconnect();
            }
            return;
        }

        // ---------- Logical transitions (online) ----------
        switch (m_appState) {
            case AppState::IDLE:
                if (event == Events::START) {
                    m_appState = AppState::STARTED;
                    enterStarted();
                }
                break;

            case AppState::STARTED:
                // All STARTED transitions are processed in the NET_* block above
                break;

            case AppState::AUTHENIFICATION:
                if (event == Events::AUTH_DATA_READY) {
                    m_appState = AppState::IDLE_AUTHENIFICATION;
                    enterIdleAuthenification();
                } else if (event == Events::AUTH_SUCCESS) {
                    // A rare path: the response came very quickly (the buffer was ready).
                    m_appState = AppState::DBLOADING;
                    enterDbLoading();
                }
                break;

            case AppState::IDLE_AUTHENIFICATION:
                if (event == Events::AUTH_SUCCESS) {
                    m_appState = AppState::DBLOADING;
                    enterDbLoading();
                } else if (event == Events::AUTH_FAIL) {
                    m_authMessage.reset();
                    emit signalAuthed(false);
                    m_appState = AppState::AUTHENIFICATION;
                    enterAuthenification();
                }
                break;

            case AppState::DBLOADING:
                if (event == Events::DB_LOADED) {
                    m_appState = AppState::CONFIGURATING;
                    enterConfigurating();
                }
                break;

            case AppState::CONFIGURATING:
                if (event == Events::CONFIG_DONE) {
                    m_appState = AppState::KCONNECTOR;
                    enterKafkaConnector();
                }
                break;

            case AppState::KCONNECTOR:
                // offset from KafkaConnector → transition to KAFKA.
                if (event == Events::OFFSET_RECEIVED) {
                    m_appState = AppState::KAFKA;
                    // KAFKA - network state: entry is executed only if online.
                    // If offline, entry will be repeated when the network returns via
                    // reenterOnReconnect().
                    if (m_netState == NetState::ONLINE) {
                        enterKafka();
                    } else {
                        qDebug() << "AppManager: KAFKA deferred — network OFFLINE";
                    }
                }
                break;

            case AppState::KAFKA:
                if (event == Events::OFFSET_ACTUAL) {
                    m_appState = AppState::READY;
                    enterReady();
                } else if (event == Events::OFFSET_STALE) {
                    m_appState = AppState::DBCLEAR;
                    enterDbClear();
                }
                break;

            case AppState::DBCLEAR:
                if (event == Events::DB_CLEARED) {
                    m_appState = AppState::SYNC;
                    if (m_netState == NetState::ONLINE) {
                        enterSync();
                    } else {
                        qDebug() << "AppManager: SYNC deferred — network OFFLINE";
                    }
                }
                break;

            case AppState::SYNC:
                if (event == Events::SYNC_DONE) {
                    m_appState = AppState::READY;
                    enterReady();
                }
                break;

            case AppState::READY:
                if (event == Events::LOGOUT) {
                    m_user.reset();
                    m_authMessage.reset();
                    m_lastKafkaOffset.clear();
                    emit signalClearResources();
                    emit signalLoggedOut();
                    m_appState = AppState::IDLE_AUTHENIFICATION;
                    enterIdleAuthenification();
                }
                break;

            case AppState::SHUTDOWN:
                // Final state
                break;
        }
    }

    // ========== SLOTS ==========

    void AppManager::onConnectionUpdated(bool state)
    {
        qDebug() << "AppManager::onConnectionUpdated(" << state << ")";
        ProcessEvent(state ? Events::NET_CONNECTED : Events::NET_DISCONNECTED);
    }

    void AppManager::onResourcesLoaded()
    {
        qDebug() << "AppManager::onResourcesLoaded()";
        ProcessEvent(Events::DB_LOADED);
    }

    void AppManager::onConfigured()
    {
        qDebug() << "AppManager::onConfigured()";
        ProcessEvent(Events::CONFIG_DONE);
    }

    void AppManager::onDatabaseCleared()
    {
        qDebug() << "AppManager::onDatabaseCleared()";
        ProcessEvent(Events::DB_CLEARED);
    }

    void AppManager::onLogout()
    {
        qDebug() << "AppManager::onLogout()";
        ProcessEvent(Events::LOGOUT);
    }

    void AppManager::onShutDown()
    {
        qDebug() << "AppManager::onShutDown()";
        ProcessEvent(Events::SHUTDOWN);
    }

    void AppManager::onKafkaOffsetReceived(QString offset)
    {
        qDebug() << "AppManager::onKafkaOffsetReceived(" << offset << ")";
        m_lastKafkaOffset = std::move(offset);
        ProcessEvent(Events::OFFSET_RECEIVED);
    }

    // Routing of incoming messages (see docs/appmanager_routing.md - top).
    //
    // Message sources: ServerConnector (server responses),
    // KafkaConnector (CRUD NOTIFICATION), MinIOConnector (GET/PUT responses).
    //
    // Rules:
    //   READY + CRUD  (subsystem != PROTOCOL) → DataManager
    //                                           (signalRecvUniterMessage)
    //   READY + PROTOCOL + MinIO (GET_MINIO_PRESIGNED_URL / GET_MINIO_FILE
    //                              / PUT_MINIO_FILE)            → FileManager
    //                                           (signalForwardToFileManager)
    // !=READY + PROTOCOL → initialization part → FSM (handleInitProtocolMessage)
    // The rest is ignored with the diagnostic log.
    void AppManager::onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message)
    {
        if (!Message) {
            return;
        }
        if (m_netState == NetState::OFFLINE) {
            qDebug() << "AppManager::onRecvUniterMessage(): ignoring in OFFLINE";
            return;
        }

        if (Message->subsystem == contract::Subsystem::PROTOCOL) {
            if (m_appState == AppState::READY) {
                handleReadyProtocolMessage(Message);
            } else {
                handleInitProtocolMessage(Message);
            }
            return;
        }

        // All non-PROTOCOL messages (CRUD) are relevant only in READY.
        if (m_appState == AppState::READY) {
            handleReadyCrudMessage(Message);
        } else {
            qDebug() << "AppManager::onRecvUniterMessage(): CRUD ignored, not READY";
        }
    }

    // --- Auxiliary: MinIO protocol actions ---
    bool AppManager::isMinioProtocolAction(contract::ProtocolAction a)
    {
        return a == contract::ProtocolAction::GET_MINIO_PRESIGNED_URL ||
               a == contract::ProtocolAction::GET_MINIO_FILE ||
               a == contract::ProtocolAction::PUT_MINIO_FILE;
    }

    // Protocol messages before READY:
    //   AUTH (RESPONSE)                 — AUTH_SUCCESS / AUTH_FAIL
    //   GET_KAFKA_CREDENTIALS (RESPONSE) — OFFSET_ACTUAL / OFFSET_STALE
    //   FULL_SYNC (SUCCESS)             — SYNC_DONE
    void AppManager::handleInitProtocolMessage(std::shared_ptr<contract::UniterMessage> message)
    {
        // Authorization
        if (message->protact == contract::ProtocolAction::AUTH &&
            message->status  == contract::MessageStatus::RESPONSE)
        {
            if (message->error == contract::ErrorCode::SUCCESS && message->resource) {
                m_user = std::dynamic_pointer_cast<contract::manager::Employee>(message->resource);
                qDebug() << "AppManager: auth SUCCESS";
                ProcessEvent(Events::AUTH_SUCCESS);
            } else {
                qDebug() << "AppManager: auth FAIL (" << message->error << ")";
                ProcessEvent(Events::AUTH_FAIL);
            }
            return;
        }

        // Checking the relevance of offset - the server returns
        // GET_KAFKA_CREDENTIALS (RESPONSE) with add_data["offset_actual"] = "true"/"false".
        if (message->protact == contract::ProtocolAction::GET_KAFKA_CREDENTIALS &&
            message->status  == contract::MessageStatus::RESPONSE &&
            m_appState      == AppState::KAFKA)
        {
            auto it = message->add_data.find("offset_actual");
            bool actual = (it != message->add_data.end() && it->second == "true");
            ProcessEvent(actual ? Events::OFFSET_ACTUAL : Events::OFFSET_STALE);
            return;
        }

        // Completion of FULL_SYNC - the server sends FULL_SYNC (SUCCESS) after
        // as the entire CREATE thread is finished.
        if (message->protact == contract::ProtocolAction::FULL_SYNC &&
            message->status  == contract::MessageStatus::SUCCESS &&
            m_appState      == AppState::SYNC)
        {
            ProcessEvent(Events::SYNC_DONE);
            return;
        }

        qDebug() << "AppManager: unexpected protocol message before READY, ignored."
                 << "protact=" << message->protact
                 << "status="  << message->status;
    }

    // Protocol messages in READY - path to FileManager for MINIO,
    // other (UPDATE_*, GET_KAFKA_CREDENTIALS refresh) are currently being logged.
    void AppManager::handleReadyProtocolMessage(std::shared_ptr<contract::UniterMessage> message)
    {
        if (isMinioProtocolAction(message->protact)) {
            emit signalForwardToFileManager(message);
            return;
        }

        switch (message->protact) {
            case contract::ProtocolAction::GET_KAFKA_CREDENTIALS:
                // Notifications about replacing credentials - TODO
                qDebug() << "AppManager: READY Kafka credentials update (not implemented)";
                break;

            case contract::ProtocolAction::UPDATE_CHECK:
            case contract::ProtocolAction::UPDATE_CONSENT:
            case contract::ProtocolAction::UPDATE_DOWNLOAD:
                // TODO: UpdaterWorker
                qDebug() << "AppManager: READY Update protocol (not implemented)";
                break;

            default:
                qDebug() << "AppManager: unhandled READY protocol action="
                         << message->protact;
                break;
        }
    }

    void AppManager::handleReadyCrudMessage(std::shared_ptr<contract::UniterMessage> message)
    {
        // CRUD in READY usually comes from KafkaConnector (NOTIFICATION).
        emit signalRecvUniterMessage(message);
    }

    // Routing outgoing messages (see docs/appmanager_routing.md - down).
    //
    // Message sources:
    // - FSM itself (enterAuthenification/enterKafka/enterSync) - before READY
    // with the already selected signalSendToServer channel directly;
    // - UI/AuthWidget - AUTH REQUEST in AUTHENIFICATION;
    // - UI widgets and FileManager - in READY, CRUD/PROTOCOL-MINIO.
    //
    // Dispatch rules in READY:
    //   CRUD (subsystem != PROTOCOL)                           → ServerConnector
    //   PROTOCOL + GET_MINIO_PRESIGNED_URL                     → ServerConnector
    //   PROTOCOL + GET_MINIO_FILE / PUT_MINIO_FILE             → MinIOConnector
    // PROTOCOL + other (UPDATE_*, GET_KAFKA_CREDENTIALS refresh) → ServerConnector
    void AppManager::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message)
    {
        if (!Message) {
            return;
        }

        if (m_appState == AppState::READY) {
            if (m_netState == NetState::OFFLINE) {
                qDebug() << "AppManager::onSendUniterMessage(): READY offline — drop for now";
                return;
            }
            dispatchOutgoing(Message);
            return;
        }

        // Before READY: the slot is called (a) from UI/AuthWidget - only AUTH REQUEST,
        // (b) from FSM itself - GET_KAFKA_CREDENTIALS/FULL_SYNC through its enter-actions
        // (they write directly to signalSendToServer and do not end up here).
        if (Message->subsystem == contract::Subsystem::PROTOCOL &&
            Message->protact   == contract::ProtocolAction::AUTH &&
            Message->status    == contract::MessageStatus::REQUEST)
        {
            // (1) Regular way: we at AUTHENIFICATION and online send
            // request and go to IDLE_AUTHENIFICATION to wait for a response.
            if (m_appState == AppState::AUTHENIFICATION && m_netState == NetState::ONLINE) {
                m_authMessage = Message;
                qDebug() << "AppManager: auth request sent from AUTHENIFICATION";
                emit signalSendToServer(m_authMessage);
                ProcessEvent(Events::AUTH_DATA_READY);
                return;
            }

            // (2) Re-login after LOGOUT: we are in IDLE_AUTHENIFICATION
            // (it does not have an entry-action, so there will be no auto-login),
            // m_authMessage was reset by onLogout. We allow sending.
            // If m_authMessage is already set - the request is “in flight”,
            // We do not accept new ones (spam protection).
            if (m_appState == AppState::IDLE_AUTHENIFICATION &&
                m_netState == NetState::ONLINE &&
                !m_authMessage)
            {
                m_authMessage = Message;
                qDebug() << "AppManager: auth request sent from IDLE_AUTHENIFICATION (re-login)";
                emit signalSendToServer(m_authMessage);
                return;
            }

            // (3) Otherwise - buffer before moving to AUTHENIFICATION
            // (for example, offline or also STARTED). enterAuthenification
            // will pick up m_authMessage and send it itself.
            m_authMessage = Message;
            qDebug() << "AppManager: auth request buffered (state="
                     << static_cast<int>(m_appState) << ")";
            return;
        }

        qDebug() << "AppManager::onSendUniterMessage(): rejected (state="
                 << static_cast<int>(m_appState)
                 << ", subsystem=" << Message->subsystem
                 << ", protact="   << Message->protact
                 << ", crudact="   << Message->crudact
                 << ")";
    }

    // Outbox Manager in READY.
    void AppManager::dispatchOutgoing(std::shared_ptr<contract::UniterMessage> message)
    {
        // Non-protocol - always to the server.
        if (message->subsystem != contract::Subsystem::PROTOCOL) {
            emit signalSendToServer(message);
            return;
        }

        switch (message->protact) {
            case contract::ProtocolAction::GET_MINIO_FILE:
            case contract::ProtocolAction::PUT_MINIO_FILE:
                emit signalSendToMinio(message);
                return;

            case contract::ProtocolAction::GET_MINIO_PRESIGNED_URL:
                // Presigned URL is issued by the server, not MinIO.
                emit signalSendToServer(message);
                return;

            default:
                // AUTH/GET_KAFKA_CREDENTIALS/FULL_SYNC/UPDATE_* - all to the server.
                emit signalSendToServer(message);
                return;
        }
    }

} // namespace uniter::control
