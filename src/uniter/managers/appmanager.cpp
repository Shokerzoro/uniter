
#include "appmanager.h"
#include "../messages/unitermessage.h"
#include "../resources/employee/employee.h"
#include <QScopedPointer>
#include <QDebug>
#include <optional>
#include <stdexcept>

namespace uniter::managers {

// Запуск приложения
void AppManager::start_run() {
    SetAppState(AppState::STARTED);
}


// Все действия при переходе в новое состояние
void AppManager::SetAppState(AppState NewState) {
    // Всегда устанавливаем состояние
    // А действия производим только в online (кроме STARTED)
    AState = NewState;

    if (NState == NetState::OFFLINE && NewState != AppState::STARTED)
        return;

    switch(NewState) {
     case AppState::IDLE:
        // Разрыв соединения и т.д.
        break;
     case AppState::STARTED:
         // Запрос на подключение
         emit signalMakeConnection();
         break;
    case AppState::CONNECTED:
        emit signalConnected();
        if(AuthMessage) { // Если AuthMessage нет, запрашиваем
            emit signalSendUniterMessage(AuthMessage);
            break;
        }
        emit signalFindAuthData();
        break;
    case AppState::AUTHENTIFICATED:
        emit signalAuthed(true); // Переключение на рабочий виджет
        emit signalConfigProc(User); // Вызов ConfigManager
        break;
    case AppState::CONFIGURATED:
        emit signalCustomizeProc(); // Запрос на применение локальных настроек
        break;
    case AppState::READY:
        emit signalPollMessages(); // Запрос на POLL сообщений
        break;
    case AppState::SHUTDOWN:

        break;
    default:
        throw std::runtime_error("AppManager::SetAppState():: error state");
        break;
    }
}

// Не проверяет валидность переходов, только отрабатывает
void AppManager::SetNetState(NetState NewState) {

    NState = NewState;

    switch (NewState) {
    case NetState::OFFLINE:
        emit signalDisconnected(); // Блокировка UI (виджет переподключения)
        break;

    case NetState::ONLINE:
        // Если возникло при запуске, просто переходим в Connected
        if(AState == AppState::STARTED) {
            SetAppState(AppState::CONNECTED);
            break;
        }
        if (AState != AppState::CONNECTED) {
            emit signalConnected();
        }
        SetAppState(AState);
        break;

    default:
        throw std::runtime_error("AppManager::SetNetState(): error state");
        break;
    }
}


// Маршрутизация вниз
void AppManager::onRecvUniterMessage(std::shared_ptr<messages::UniterMessage> Message) {

    // Пересылка обычных crud
    if(AState == AppState::READY && Message->subsystem != messages::Subsystem::PROTOCOL) {
        emit signalRecvUniterMessage(Message);
    }

    // Протокольные сообщения в ready состоянии
    if(AState == AppState::READY && Message->subsystem == messages::Subsystem::PROTOCOL) {
        // TODO: обновление конфигурации user

        // TODO: то что связано с sync - синхронизацией

        // TODO: удаление пользователя
    }

    // Протокольные сообщения во время инициализации
    if(AState != AppState::READY && Message->subsystem == messages::Subsystem::PROTOCOL) {

        // Если ждем аутентификацию
        if (AState == AppState::CONNECTED && Message->protact == messages::ProtocolAction::GETCONFIG) {
            if(Message->status == messages::MessageStatus::RESPONSE) {

                if(Message->error == messages::ErrorCode::SUCCESS && Message->resource) {
                    User = std::dynamic_pointer_cast<resources::employees::Employee>(Message->resource);
                    SetAppState(AppState::AUTHENTIFICATED);
                    return;
                }
                else if(Message->error == messages::ErrorCode::BAD_REQUEST) {
                    emit signalAuthed(false);
                }

            }
        }

        // Если необходимо выполнять sync перед переходом в ready
        // Ответ на POLL - bad_request

    }
}

// Маршрутизация вверх
void AppManager::onSendUniterMessage(std::shared_ptr<messages::UniterMessage> Message) {

    // Пересылка обычных crud
    if (AState == AppState::READY) {
        emit signalSendUniterMessage(Message);
    }

    // Протокольные сообщения во время appstate::ready
    if (AState == AppState::READY && Message->subsystem == messages::Subsystem::PROTOCOL) {
        // Обработка изменения конфига

        // Обработка выхода (например пользователь удален)

    }

    // Протокольные сообщения во время инициализации
    if (AState != AppState::READY && Message->subsystem == messages::Subsystem::PROTOCOL) {

        // Обработка запроса на аутентификацию
        if(Message->protact == messages::ProtocolAction::GETCONFIG &&
            Message->status == messages::MessageStatus::REQUEST) {
            AuthMessage = std::move(Message);

            // Если CONNECTED и ONLINE записываем и отправляем
            if(AState == AppState::CONNECTED && NState == NetState::ONLINE) {
                emit signalSendUniterMessage(AuthMessage);
            }
        }

    }

}

// Сигналы от сетевого класса
void AppManager::onConnected() {
    SetNetState(NetState::ONLINE);
}

void AppManager::onDisconnected() {
    SetNetState(NetState::OFFLINE);
}

// Сигналы от менеджеров
void AppManager::onConfigured() {
    if(AState == AppState::AUTHENTIFICATED) {
        SetAppState(AppState::CONFIGURATED);
    }
}

void AppManager::onCustomized() {
    if(AState == AppState::CONFIGURATED) {
        SetAppState(AppState::READY);
    }
}

void AppManager::onResourcesLoaded() {

}

void AppManager::onShutDown() {
    // TODO: выполнить сохранение
    // TODO: на выход регестрируем слот shutdown
}



} // managers
