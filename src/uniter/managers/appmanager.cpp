
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
    QDebug("AppManager::start_run()");
    SetAppState(AppState::STARTED);
}


// Все действия при переходе в новое состояние
// Не проверяет валидность переходов, только отрабатывает
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
            AuthMessage.reset(); // Сразу очищаем данные
            break;
        }
        emit signalFindAuthData();
        break;
    case AppState::AUTHENTIFICATED:
        emit signalAuthed(true); // Переключение на рабочий виджет
        emit signalConfigProc(std::move(User)); // Вызов ConfigManager
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


// Маршрутизация сообщений
// Точка входа для данных (могут приходить в offline? Фактически нет)
void AppManager::onRecvUniterMessage(QScopedPointer<messages::UniterMessage> Message) {

    // Обработка инициализационных сообщений (только UniterMessage с User)
    if (AState != AppState::READY && Message->subsystem == messages::Subsystem::PROTOCOL) {
        switch(AState) {
        case AppState::CONNECTED:
            // Получение аутентификации
            if(Message->protact == messages::ProtocolAction::GETCONFIG && Message->status == messages::MessageStatus::RESPONSE) {

                if(Message->error == messages::ErrorCode::SUCCESS) { // Успешная аутентификация
                    SetAppState(AppState::AUTHENTIFICATED);
                }
                else { // Неуспешная аутентификация
                    emit signalAuthed(false);
                }
            )
            break;
        default:
            // Не понятно что делать, ошибка какая-то
            break;
        }
        return;
    }

    // Обработка основных CRUD сообщений (передача выше)
    if (AState == AppState::READY && Message->subsystem != messages::Subsystem::PROTOCOL) {

        return;
    }

    // Обработка остальных сообщений (исключительный случай)
    // Например runtime изменение конфигурации/данный пользователя
    if (AState == AppState::READY && Message->subsystem == messages::Subsystem::PROTOCOL) {
        if (Message->protact == messages::ProtocolAction::GETCONFIG && Message->status == messages::MessageStatus::RESPONSE) {
            //TODO: Изменился User, создаем заново/изменяем
        }
    }


} // onRecvUniterMessage


// Минимальная проверка (состояние, права)? Или отдавать сетевому классу чтобы он буферезировал
void AppManager::onSendUniterMeassage(QScopedPointer<messages::UniterMessage> Message) {

    // Обработка инициализационных сообщений
    if (AState != AppState::READY && Message->subsystem == messages::Subsystem::PROTOCOL) {
        // Если запрос авторизация, то сохранение (если не AUTHENTIFICATED) или передача
        // Что еще может быть????
    }

    // Обработка основных сообщений
    if (AState == AppState::READY && Message->subsystem != messages::Subsystem::PROTOCOL) {
        // Если проверять права, то это нужно целиком парсить сообщение.
        // Может быть проще просто настроить UI правильно?
        // Но тогда дыра в безопасности при ошибках в UI
    }

} // onSendUniterMeassage



// public slots (выделены для инкапсуляции AState NState)
// От сетевого класса
void AppManager::onConnected() {
    if(NState == NetState::OFFLINE)
        SetNetState(NetState::ONLINE);
}
void AppManager::onDisconnected() {
    if(NState == NetState::ONLINE)
        SetNetState(NetState::OFFLINE);
}

// От менеджеров / выключение
void AppManager::onConfigured() {
    if(AState == AppState::AUTHENTIFICATED)
        SetAppState(AppState::CONFIGURATED);
}
void AppManager::onCustomized() {
    if(AState == AppState::CONFIGURATED)
        SetAppState(AppState::READY);
}
void AppManager::onShutDown() {
    // Потом можно добавить виджет с вопосом
    SetAppState(AppState::SHUTDOWN);
}

} // managers
