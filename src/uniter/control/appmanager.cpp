
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

AppManager* AppManager::instance() {
    static AppManager instance;
    return &instance;
}

AppManager::AppManager() {
    // Конструктор пустой - все связи устанавливаются в main.cpp
}

void AppManager::start_run() {
    ProcessEvent(Events::START);
}

void AppManager::ProcessEvent(Events event) {

    // Обработчики входа в состояния
    static auto in_idle = [](AppManager* mgr) {
        mgr->m_appState = AppState::IDLE;
        qDebug() << "AppManager::in_idle(): entered IDLE state";
    };

    static auto in_started = [](AppManager* mgr) {
        mgr->m_appState = AppState::STARTED;
        qDebug() << "AppManager::in_started(): entered STARTED state";
        emit mgr->signalMakeConnection();
    };

    static auto in_connected = [](AppManager* mgr) {
        mgr->m_appState = AppState::CONNECTED;
        qDebug() << "AppManager::in_connected(): entered CONNECTED state";
        emit mgr->signalConnected();

        // Если есть сохраненное сообщение аутентификации - отправляем
        if (mgr->m_authMessage) {
            qDebug() << "AppManager::in_connected(): sending stored auth request";
            emit mgr->signalSendUniterMessage(mgr->m_authMessage);
        } else {
            // Иначе запрашиваем данные для аутентификации
            emit mgr->signalFindAuthData();
            qDebug() << "AppManager::in_connected(): emitted signalFindAuthData";
        }
    };

    static auto in_authentificated = [](AppManager* mgr) {
        mgr->m_appState = AppState::AUTHENTIFICATED;
        qDebug() << "AppManager::in_authentificated(): entered AUTHENTIFICATED state";
        emit mgr->signalAuthed(true);

        // Запуск инициализации БД
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (mgr->m_user) {
            QString userName = mgr->m_user->name + mgr->m_user->surname + mgr->m_user->patronymic;
            hash.addData(userName.toUtf8());
        }
        emit mgr->signalLoadResources(hash.result());
    };

    static auto in_dbloaded = [](AppManager* mgr) {
        mgr->m_appState = AppState::DBLOADED;
        qDebug() << "AppManager::in_dbloaded(): entered DBLOADED state";
        emit mgr->signalConfigProc(mgr->m_user);
    };

    static auto in_ready = [](AppManager* mgr) {
        mgr->m_appState = AppState::READY;
        qDebug() << "AppManager::in_ready(): entered READY state";
        emit mgr->signalPollMessages();
    };

    static auto in_shutdown = [](AppManager* mgr) {
        mgr->m_appState = AppState::SHUTDOWN;
        qDebug() << "AppManager::in_shutdown(): entered SHUTDOWN state";
        // TODO: сохранение настроек и буферов
    };

    // Обработчики выхода из состояний
    static auto out_idle = [](AppManager* mgr) {
        qDebug() << "AppManager::out_idle(): leaving IDLE state";
    };

    static auto out_started = [](AppManager* mgr) {
        qDebug() << "AppManager::out_started(): leaving STARTED state";
    };

    static auto out_connected = [](AppManager* mgr) {
        qDebug() << "AppManager::out_connected(): leaving CONNECTED state";
    };

    static auto out_authentificated = [](AppManager* mgr) {
        qDebug() << "AppManager::out_authentificated(): leaving AUTHENTIFICATED state";
    };

    static auto out_dbloaded = [](AppManager* mgr) {
        qDebug() << "AppManager::out_dbloaded(): leaving DBLOADED state";
    };

    static auto out_ready = [](AppManager* mgr) {
        qDebug() << "AppManager::out_ready(): leaving READY state";
    };

    static auto out_shutdown = [](AppManager* mgr) {
        qDebug() << "AppManager::out_shutdown(): leaving SHUTDOWN state";
    };

    // Обработчики самопереходов при восстановлении сети
    static auto self_connected = [](AppManager* mgr) {
        qDebug() << "AppManager::self_connected(): network restored in CONNECTED state";
        // Переотправляем запрос аутентификации если он есть
        if (mgr->m_authMessage) {
            emit mgr->signalSendUniterMessage(mgr->m_authMessage);
        }
    };

    static auto self_authentificated = [](AppManager* mgr) {
        qDebug() << "AppManager::self_authentificated(): network restored in AUTHENTIFICATED state";
        // Можем повторить инициализацию БД если нужно
    };

    static auto self_dbloaded = [](AppManager* mgr) {
        qDebug() << "AppManager::self_dbloaded(): network restored in DBLOADED state";
        // Можем повторить запрос конфигурации если нужно
    };

    static auto self_ready = [](AppManager* mgr) {
        qDebug() << "AppManager::self_ready(): network restored in READY state";
        emit mgr->signalPollMessages();
    };

    // Обработка сетевых событий отдельно - они влияют на NetState
    if (event == Events::NET_CONNECTED) {
        qDebug() << "AppManager::ProcessEvent(): NET_CONNECTED event";
        m_netState = NetState::ONLINE;
        emit signalConnected();

        // Самопереходы в зависимости от текущего состояния приложения
        switch (m_appState) {
        case AppState::STARTED:
            // Только из STARTED сетевое подключение переводит в CONNECTED
            out_started(this);
            in_connected(this);
            return;
        case AppState::CONNECTED:
            self_connected(this);
            return;
        case AppState::AUTHENTIFICATED:
            self_authentificated(this);
            return;
        case AppState::DBLOADED:
            self_dbloaded(this);
            return;
        case AppState::READY:
            self_ready(this);
            return;
        case AppState::IDLE:
        case AppState::SHUTDOWN:
            // В этих состояниях просто фиксируем подключение
            return;
        }
        return;
    }

    if (event == Events::NET_DISCONNECTED) {
        qDebug() << "AppManager::ProcessEvent(): NET_DISCONNECTED event";
        m_netState = NetState::OFFLINE;
        emit signalDisconnected();
        // AppState не меняется, только блокируем UI
        return;
    }

    // Обработка событий приложения (требуют ONLINE, кроме START и SHUTDOWN)
    if (event != Events::START && event != Events::SHUTDOWN && m_netState == NetState::OFFLINE) {
        qDebug() << "AppManager::ProcessEvent(): ignoring event in OFFLINE state";
        return;
    }

    switch (m_appState) {
    case AppState::IDLE:
        if (event == Events::START) {
            out_idle(this);
            in_started(this);
            return;
        }
        break;

    case AppState::STARTED:
        // Переход в CONNECTED происходит только через NET_CONNECTED (обработан выше)
        if (event == Events::SHUTDOWN) {
            out_started(this);
            in_shutdown(this);
            return;
        }
        break;

    case AppState::CONNECTED:
        if (event == Events::AUTH_SUCCESS) {
            out_connected(this);
            in_authentificated(this);
            return;
        }
        if (event == Events::SHUTDOWN) {
            out_connected(this);
            in_shutdown(this);
            return;
        }
        break;

    case AppState::AUTHENTIFICATED:
        if (event == Events::DB_LOADED) {
            out_authentificated(this);
            in_dbloaded(this);
            return;
        }
        if (event == Events::SHUTDOWN) {
            out_authentificated(this);
            in_shutdown(this);
            return;
        }
        break;

    case AppState::DBLOADED:
        if (event == Events::CONFIG_DONE) {
            out_dbloaded(this);
            in_ready(this);
            return;
        }
        if (event == Events::SHUTDOWN) {
            out_dbloaded(this);
            in_shutdown(this);
            return;
        }
        break;

    case AppState::READY:
        if (event == Events::SHUTDOWN) {
            out_ready(this);
            in_shutdown(this);
            return;
        }
        break;

    case AppState::SHUTDOWN:
        // Финальное состояние - переходы не допускаются
        qDebug() << "AppManager::ProcessEvent(): already in SHUTDOWN, ignoring event";
        return;

    default:
        throw std::runtime_error("AppManager::ProcessEvent(): unknown state");
    }

    // Если дошли сюда - переход не допустим
    qDebug() << "AppManager::ProcessEvent(): invalid transition from current state for given event";
}

// Слоты от сетевого класса
void AppManager::onConnected() {
    qDebug() << "AppManager::onConnected()";
    ProcessEvent(Events::NET_CONNECTED);
}

void AppManager::onDisconnected() {
    qDebug() << "AppManager::onDisconnected()";
    ProcessEvent(Events::NET_DISCONNECTED);
}

void AppManager::onResourcesLoaded() {
    qDebug() << "AppManager::onResourcesLoaded()";
    ProcessEvent(Events::DB_LOADED);
}

void AppManager::onConfigured() {
    qDebug() << "AppManager::onConfigured()";
    ProcessEvent(Events::CONFIG_DONE);
}

void AppManager::onShutDown() {
    qDebug() << "AppManager::onShutDown()";
    ProcessEvent(Events::SHUTDOWN);
}

// Маршрутизация входящих сообщений
void AppManager::onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> Message) {


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

                // Разблокируем mutex перед вызовом ProcessEvent (он сам захватит mutex)
                ProcessEvent(Events::AUTH_SUCCESS);
                return; // mutex уже разблокирован, выходим

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
void AppManager::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> Message) {

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

} // control
