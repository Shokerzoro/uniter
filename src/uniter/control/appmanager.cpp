
#include "appmanager.h"
#include "configmanager.h"
#include "../contract/unitermessage.h"
#include "../contract/employee/employee.h"

#include <QCryptographicHash>
#include <QDebug>
#include <optional>
#include <stdexcept>
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

    // ========== ОБРАБОТКА СОБЫТИЙ FSM ==========

    void AppManager::ProcessEvent(Events event)
    {
        qDebug() << "AppManager::ProcessEvent() - Event received";

        // Лямбды для переходов между состояниями
        auto in_idle = []() {
            qDebug() << "AppManager::in_idle()";
        };

        auto out_idle = []() {
            qDebug() << "AppManager::out_idle()";
        };

        auto in_started = [this]() {
            qDebug() << "AppManager::in_started()";
            emit signalMakeConnection();
        };

        auto out_started = []() {
            qDebug() << "AppManager::out_started()";
        };

        auto in_idle_connected = [this]() {
            qDebug() << "AppManager::in_idle_connected()";
            m_netState = NetState::ONLINE;
            emit signalConnectionUpdated(true);
        };

        auto in_connected = [this]() {
            qDebug() << "AppManager::in_connected()";
            m_netState = NetState::ONLINE;
            emit signalConnectionUpdated(true);

            emit signalFindAuthData();         // ← только здесь
            if (m_authMessage) {
                qDebug() << "AppManager::in_connected(): sending stored auth request";
                emit signalSendUniterMessage(m_authMessage);
            }
        };


        auto out_connected = []() {
            qDebug() << "AppManager::out_connected()";
        };

        auto in_authentificated = [this]() {
            qDebug() << "AppManager::in_authentificated()";
            emit signalAuthed(true);

            if (m_user) {
                QByteArray userhash = QByteArray::number(m_user->id);
                emit signalLoadResources(userhash);
            }
        };

        auto out_authentificated = []() {
            qDebug() << "AppManager::out_authentificated()";
        };

        auto in_dbloaded = [this]() {
            qDebug() << "AppManager::in_dbloaded()";
            emit signalConfigProc(m_user);
        };

        auto out_dbloaded = []() {
            qDebug() << "AppManager::out_dbloaded()";
        };

        auto in_ready = []() {
            qDebug() << "AppManager::in_ready()";
        };

        auto out_ready = []() {
            qDebug() << "AppManager::out_ready()";
        };

        auto in_offline = [this]() {
            qDebug() << "AppManager::in_offline()";
            m_netState = NetState::OFFLINE;
            emit signalConnectionUpdated(false);
        };

        auto out_offline = []() {
            qDebug() << "AppManager::out_offline()";
        };

        auto in_shutdown = []() {
            qDebug() << "AppManager::in_shutdown()";
        };

        auto out_shutdown = []() {
            qDebug() << "AppManager::out_shutdown()";
        };

        // Обработка событий в зависимости от текущего состояния
        switch (m_appState) {
            case AppState::IDLE:
                if (event == Events::START) {
                    out_idle();
                    m_appState = AppState::STARTED;
                    in_started();
                }
                break;

            case AppState::STARTED:
                if (event == Events::NET_CONNECTED) {
                    out_started();
                    m_appState = AppState::CONNECTED;
                    in_connected();
                }
                else if (event == Events::NET_DISCONNECTED) {
                    out_started();
                    m_appState = AppState::STARTED;
                    in_offline();
                }
                break;

            case AppState::CONNECTED:
                if (event == Events::AUTH_SUCCESS) {
                    out_connected();
                    m_appState = AppState::AUTHENTIFICATED;
                    in_authentificated();
                }
                else if (event == Events::NET_DISCONNECTED) {
                    out_connected();
                    m_appState = AppState::CONNECTED;
                    in_offline();
                }
                break;

            case AppState::AUTHENTIFICATED:
                if (event == Events::DB_LOADED) {
                    out_authentificated();
                    m_appState = AppState::DBLOADED;
                    in_dbloaded();
                }
                else if (event == Events::NET_DISCONNECTED) {
                    out_authentificated();
                    m_appState = AppState::AUTHENTIFICATED;
                    in_offline();
                }
                break;

            case AppState::DBLOADED:
                if (event == Events::CONFIG_DONE) {
                    out_dbloaded();
                    m_appState = AppState::READY;
                    in_ready();
                }
                else if (event == Events::NET_DISCONNECTED) {
                    out_dbloaded();
                    m_appState = AppState::DBLOADED;
                    in_offline();
                }
                break;

            case AppState::READY:
                if (event == Events::LOGOUT) {
                    out_ready();
                    // Очистка ресурсов
                    m_user.reset();
                    m_authMessage.reset();
                    emit signalClearResources();
                    emit signalLoggedOut();
                    m_appState = AppState::CONNECTED;
                    in_idle_connected();
                }
                else if (event == Events::NET_DISCONNECTED) {
                    out_ready();
                    m_appState = AppState::READY;
                    in_offline();
                }
                else if (event == Events::SHUTDOWN) {
                    out_ready();
                    m_appState = AppState::SHUTDOWN;
                    in_shutdown();
                }
                break;

            case AppState::SHUTDOWN:
                // Финальное состояние
                break;
        }

        // Обработка восстановления соединения (независимо от AppState)
        if (event == Events::NET_CONNECTED && m_netState == NetState::OFFLINE) {
            out_offline();

            switch (m_appState) {
                case AppState::STARTED:
                    in_started();
                    break;
                case AppState::IDLE_CONNECTED:
                    in_idle_connected();
                    break;
                case AppState::CONNECTED:
                    in_connected();
                    break;
                case AppState::AUTHENTIFICATED:
                    in_authentificated();
                    break;
                case AppState::DBLOADED:
                    in_dbloaded();
                    break;
                case AppState::READY:
                    in_ready();
                    break;
                default:
                    break;
            }
        }
    }

    // ========== СЛОТЫ ==========

    // Объединённый слот от сетевого класса
    void AppManager::onConnectionUpdated(bool state)
    {
        qDebug() << "AppManager::onConnectionUpdated(" << state << ")";

        if (state) {
            ProcessEvent(Events::NET_CONNECTED);
        } else {
            ProcessEvent(Events::NET_DISCONNECTED);
        }
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

    // Маршрутизация входящих сообщений
    void AppManager::onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message)
    {
        // Проверяем состояние сети
        if (m_netState == NetState::OFFLINE) {
            qDebug() << "AppManager::onRecvUniterMessage(): ignoring message in OFFLINE state";
            return;
        }

        // Пересылка обычных CRUD в состоянии READY
        if (m_appState == AppState::READY && Message->subsystem != contract::Subsystem::PROTOCOL) {
            emit signalRecvUniterMessage(Message);
            return;
        }

        // Протокольные сообщения в READY
        if (m_appState == AppState::READY && Message->subsystem == contract::Subsystem::PROTOCOL) {
            // TODO: обновление конфигурации user
            // TODO: синхронизация
            // TODO: удаление пользователя
            return;
        }

        // Протокольные сообщения во время инициализации
        if (m_appState != AppState::READY && Message->subsystem == contract::Subsystem::PROTOCOL) {

            // Ответ на аутентификацию
            if (m_appState == AppState::CONNECTED &&
                Message->protact == contract::ProtocolAction::AUTH &&
                Message->status == contract::MessageStatus::RESPONSE) {

                if (Message->error == contract::ErrorCode::SUCCESS && Message->resource) {
                    m_user = std::dynamic_pointer_cast<contract::employees::Employee>(Message->resource);
                    qDebug() << "AppManager::onRecvUniterMessage(): authentication successful";

                    ProcessEvent(Events::AUTH_SUCCESS);
                    return;

                } else if (Message->error == contract::ErrorCode::BAD_REQUEST) {
                    qDebug() << "AppManager::onRecvUniterMessage(): authentication failed";
                    // Остаемся в CONNECTED, только уведомляем UI
                    emit signalAuthed(false);
                }
                return;
            }
        }
    }

    // Маршрутизация исходящих сообщений
    void AppManager::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message)
    {
        // Проверяем состояние сети
        if (m_netState == NetState::OFFLINE) {
            qDebug() << "AppManager::onSendUniterMessage(): cannot send message in OFFLINE state";
            return;
        }

        // Пересылка обычных CRUD в состоянии READY
        if (m_appState == AppState::READY) {
            emit signalSendUniterMessage(Message);
            return;
        }

        // Протокольные сообщения в READY
        if (m_appState == AppState::READY && Message->subsystem == contract::Subsystem::PROTOCOL) {
            // TODO: обработка изменения конфига
            // TODO: обработка выхода
            return;
        }

        // Обработка запроса аутентификации
        if (Message->subsystem == contract::Subsystem::PROTOCOL &&
            Message->protact == contract::ProtocolAction::AUTH &&
            Message->status == contract::MessageStatus::REQUEST) {

            m_authMessage = Message;
            qDebug() << "AppManager::onSendUniterMessage(): stored auth request";

            // Если уже в CONNECTED - отправляем сразу
            if (m_appState == AppState::CONNECTED) {
                qDebug() << "AppManager::onSendUniterMessage(): sending auth request immediately";
                emit signalSendUniterMessage(m_authMessage);
            }
        }
    }

    } // namespace uniter::control
