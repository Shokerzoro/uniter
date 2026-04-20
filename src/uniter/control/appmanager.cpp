
#include "appmanager.h"
#include "configmanager.h"
#include "../contract/unitermessage.h"
#include "../contract/employee/employee.h"

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
        // Если данные от виджета уже в буфере — сразу отправляем.
        if (m_authMessage) {
            qDebug() << "AppManager: AUTHENIFICATION — stored authMessage present, sending";
            emit signalSendToServer(m_authMessage);
            // Переходим в IDLE_AUTHENIFICATION: ждём ответ сервера
            ProcessEvent(Events::AUTH_DATA_READY);
            return;
        }
        // Иначе — просим виджет прислать логин/пароль
        emit signalFindAuthData();
    }

    void AppManager::enterIdleAuthenification()
    {
        qDebug() << "AppManager: enter IDLE_AUTHENIFICATION — waiting server response";
        // Действий при входе нет: просто ждём ответ сервера.
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
        // KafkaConnector инициализируется под пользователя, после чего
        // пришлёт сохранённый offset через signalOffsetReady → onKafkaOffsetReceived.
        if (!m_user) {
            qDebug() << "AppManager: KCONNECTOR entered without m_user (unexpected)";
            return;
        }
        emit signalInitKafkaConnector(makeUserHash());
    }

    void AppManager::enterKafka()
    {
        qDebug() << "AppManager: enter KAFKA — requesting server offset actuality check";
        // Сетевой запрос: GET_KAFKA_CREDENTIALS REQUEST с offset в add_data.
        // В ответ сервер пришлёт RESPONSE с add_data["offset_actual"] = "true"|"false".
        auto msg = std::make_shared<contract::UniterMessage>();
        msg->subsystem = contract::Subsystem::PROTOCOL;
        msg->protact   = contract::ProtocolAction::GET_KAFKA_CREDENTIALS;
        msg->status    = contract::MessageStatus::REQUEST;
        msg->add_data.emplace("offset", m_lastKafkaOffset);
        emit signalSendToServer(msg);
    }

    void AppManager::enterDbClear()
    {
        qDebug() << "AppManager: enter DBCLEAR — clearing database";
        // Локальная операция: просим DataManager очистить все таблицы БД.
        // DataManager в ответ пришлёт signalDatabaseCleared → onDatabaseCleared.
        emit signalClearDatabase();
    }

    void AppManager::enterSync()
    {
        qDebug() << "AppManager: enter SYNC — requesting FULL_SYNC from server";
        // Сетевой запрос: FULL_SYNC REQUEST (без параметров в add_data).
        // В ответ сервер через Kafka пришлёт поток CRUD CREATE, а по окончании —
        // FULL_SYNC SUCCESS в PROTOCOL.
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
        // TODO: фактическое сохранение настроек и буферов сообщений.
    }

    void AppManager::reenterOnReconnect()
    {
        // Вызывается при NET_CONNECTED, если AppState уже внутри UI-блокируемой
        // области и состояние ЗАВИСИТ от сети.
        // Для локальных состояний ничего не делаем (при возврате сети они
        // просто продолжают выполняться).
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
                // DBCLEAR — не перезапускаются.
                break;
        }
    }

    bool AppManager::isNetworkDependent(AppState s)
    {
        // Состояния, у которых есть offline-зеркало на диаграмме.
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
        // «UI-блокируемая часть» (пунктирная рамка на диаграмме):
        // все состояния начиная с AUTHENIFICATION до READY включительно.
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
        // Stub: используем id пользователя в виде строки.
        // В будущем здесь будет реальный хэш (employee id + salt).
        if (!m_user) return {};
        return QByteArray::number(static_cast<qulonglong>(m_user->id));
    }

    // ========== FSM ==========

    void AppManager::ProcessEvent(Events event)
    {
        qDebug() << "AppManager::ProcessEvent() state=" << static_cast<int>(m_appState)
                 << " event=" << static_cast<int>(event);

        // ---------- Обработка SHUTDOWN из любого состояния ----------
        if (event == Events::SHUTDOWN) {
            m_appState = AppState::SHUTDOWN;
            enterShutdown();
            return;
        }

        // ---------- Обработка сетевых событий (онлайн/оффлайн) ----------
        // Принцип: NetState ортогонален AppState. Потеря/возврат сети не
        // меняет AppState (кроме STARTED → AUTHENIFICATION при первом коннекте).
        // Для сетевых состояний при возврате online ПОВТОРЯЕТСЯ entry-action;
        // для локальных — нет.
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
            // STARTED → AUTHENIFICATION при первом подключении.
            if (m_appState == AppState::STARTED) {
                m_appState = AppState::AUTHENIFICATION;
                enterAuthenification();
                return;
            }
            // Для сетевых состояний повторяем entry (продолжить начатое действие).
            if (was_offline && isNetworkDependent(m_appState)) {
                reenterOnReconnect();
            }
            return;
        }

        // ---------- Логические переходы (online) ----------
        switch (m_appState) {
            case AppState::IDLE:
                if (event == Events::START) {
                    m_appState = AppState::STARTED;
                    enterStarted();
                }
                break;

            case AppState::STARTED:
                // Все переходы STARTED обрабатываются в блоке NET_* выше
                break;

            case AppState::AUTHENIFICATION:
                if (event == Events::AUTH_DATA_READY) {
                    m_appState = AppState::IDLE_AUTHENIFICATION;
                    enterIdleAuthenification();
                } else if (event == Events::AUTH_SUCCESS) {
                    // Редкий путь: ответ пришёл очень быстро (буфер был готов).
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
                // offset от KafkaConnector → переход в KAFKA.
                if (event == Events::OFFSET_RECEIVED) {
                    m_appState = AppState::KAFKA;
                    // KAFKA — сетевое состояние: entry выполняем только если online.
                    // Если offline, entry повторится при возврате сети через
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
                // Финальное состояние
                break;
        }
    }

    // ========== СЛОТЫ ==========

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

    // Маршрутизация входящих сообщений (см. docs/appmanager_routing.md — вверх).
    //
    // Источники сообщений: ServerConnector (ответы сервера),
    // KafkaConnector (CRUD NOTIFICATION), MinIOConnector (ответы GET/PUT).
    //
    // Правила:
    //   READY + CRUD  (subsystem != PROTOCOL) → DataManager
    //                                           (signalRecvUniterMessage)
    //   READY + PROTOCOL + MinIO (GET_MINIO_PRESIGNED_URL / GET_MINIO_FILE
    //                              / PUT_MINIO_FILE)            → FileManager
    //                                           (signalForwardToFileManager)
    //   !=READY + PROTOCOL → часть инициализации → FSM (handleInitProtocolMessage)
    //   Остальное — игнорируется с диагностическим логом.
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

        // Все не-PROTOCOL сообщения (CRUD) актуальны только в READY.
        if (m_appState == AppState::READY) {
            handleReadyCrudMessage(Message);
        } else {
            qDebug() << "AppManager::onRecvUniterMessage(): CRUD ignored, not READY";
        }
    }

    // --- Вспомогательное: MinIO-протокольные действия ---
    bool AppManager::isMinioProtocolAction(contract::ProtocolAction a)
    {
        return a == contract::ProtocolAction::GET_MINIO_PRESIGNED_URL ||
               a == contract::ProtocolAction::GET_MINIO_FILE ||
               a == contract::ProtocolAction::PUT_MINIO_FILE;
    }

    // Протокольные сообщения до READY:
    //   AUTH (RESPONSE)                 — AUTH_SUCCESS / AUTH_FAIL
    //   GET_KAFKA_CREDENTIALS (RESPONSE) — OFFSET_ACTUAL / OFFSET_STALE
    //   FULL_SYNC (SUCCESS)             — SYNC_DONE
    void AppManager::handleInitProtocolMessage(std::shared_ptr<contract::UniterMessage> message)
    {
        // Авторизация
        if (message->protact == contract::ProtocolAction::AUTH &&
            message->status  == contract::MessageStatus::RESPONSE)
        {
            if (message->error == contract::ErrorCode::SUCCESS && message->resource) {
                m_user = std::dynamic_pointer_cast<contract::employees::Employee>(message->resource);
                qDebug() << "AppManager: auth SUCCESS";
                ProcessEvent(Events::AUTH_SUCCESS);
            } else {
                qDebug() << "AppManager: auth FAIL (" << message->error << ")";
                ProcessEvent(Events::AUTH_FAIL);
            }
            return;
        }

        // Проверка актуальности offset — сервер возвращает
        // GET_KAFKA_CREDENTIALS (RESPONSE) с add_data["offset_actual"] = "true"/"false".
        if (message->protact == contract::ProtocolAction::GET_KAFKA_CREDENTIALS &&
            message->status  == contract::MessageStatus::RESPONSE &&
            m_appState      == AppState::KAFKA)
        {
            auto it = message->add_data.find("offset_actual");
            bool actual = (it != message->add_data.end() && it->second == "true");
            ProcessEvent(actual ? Events::OFFSET_ACTUAL : Events::OFFSET_STALE);
            return;
        }

        // Завершение FULL_SYNC — сервер шлёт FULL_SYNC (SUCCESS) после того
        // как весь поток CREATE закончен.
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

    // Протокольные сообщения в READY — путь к FileManager для MINIO,
    // прочее (UPDATE_*, GET_KAFKA_CREDENTIALS refresh) пока логируем.
    void AppManager::handleReadyProtocolMessage(std::shared_ptr<contract::UniterMessage> message)
    {
        if (isMinioProtocolAction(message->protact)) {
            emit signalForwardToFileManager(message);
            return;
        }

        switch (message->protact) {
            case contract::ProtocolAction::GET_KAFKA_CREDENTIALS:
                // Уведомления о замене credentials — TODO
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
        // CRUD в READY приходит, как правило, из KafkaConnector (NOTIFICATION).
        emit signalRecvUniterMessage(message);
    }

    // Маршрутизация исходящих сообщений (см. docs/appmanager_routing.md — вниз).
    //
    // Источники сообщений:
    //   — сама FSM (enterAuthenification/enterKafka/enterSync) — до READY
    //     с уже выбранным каналом signalSendToServer напрямую;
    //   — UI/AuthWidget — AUTH REQUEST в AUTHENIFICATION;
    //   — UI-виджеты и FileManager — в READY, CRUD/PROTOCOL-MINIO.
    //
    // Правила диспетчеризации в READY:
    //   CRUD (subsystem != PROTOCOL)                           → ServerConnector
    //   PROTOCOL + GET_MINIO_PRESIGNED_URL                     → ServerConnector
    //   PROTOCOL + GET_MINIO_FILE / PUT_MINIO_FILE             → MinIOConnector
    //   PROTOCOL + иное (UPDATE_*, GET_KAFKA_CREDENTIALS refresh) → ServerConnector
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

        // До READY: слот вызывается (а) от UI/AuthWidget — только AUTH REQUEST,
        // (б) от самой FSM — GET_KAFKA_CREDENTIALS/FULL_SYNC через свои enter-actions
        // (они пишут напрямую в signalSendToServer и сюда не попадают).
        if (Message->subsystem == contract::Subsystem::PROTOCOL &&
            Message->protact   == contract::ProtocolAction::AUTH &&
            Message->status    == contract::MessageStatus::REQUEST)
        {
            m_authMessage = Message;
            qDebug() << "AppManager: stored auth request";

            // Если мы сейчас в AUTHENIFICATION и онлайн — сразу шлём и
            // переходим в IDLE_AUTHENIFICATION.
            if (m_appState == AppState::AUTHENIFICATION && m_netState == NetState::ONLINE) {
                emit signalSendToServer(m_authMessage);
                ProcessEvent(Events::AUTH_DATA_READY);
            }
            return;
        }

        qDebug() << "AppManager::onSendUniterMessage(): rejected (state="
                 << static_cast<int>(m_appState)
                 << ", subsystem=" << Message->subsystem
                 << ", protact="   << Message->protact
                 << ", crudact="   << Message->crudact
                 << ")";
    }

    // Диспетчер исходящих в READY.
    void AppManager::dispatchOutgoing(std::shared_ptr<contract::UniterMessage> message)
    {
        // Не-протокольные — всегда на сервер.
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
                // Presigned URL выдаёт сервер, а не MinIO.
                emit signalSendToServer(message);
                return;

            default:
                // AUTH/GET_KAFKA_CREDENTIALS/FULL_SYNC/UPDATE_* — все на сервер.
                emit signalSendToServer(message);
                return;
        }
    }

} // namespace uniter::control
