
#include "appmanager.h"
#include "configmanager.h"
#include "../data/datamanager.h"
#include "../network/mocknetwork.h"
#include "../messages/unitermessage.h"
#include "../resources/employee/employee.h"

#include <QCryptographicHash>
#include <QDebug>
#include <optional>
#include <stdexcept>
#include <memory>

namespace uniter::managers {


AppManager* AppManager::instance()
{
    static AppManager instance;
    return &instance;
}


AppManager::AppManager()
{
    // Конструктор пустой - все связи устанавливаются в main.cpp
}


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
        qDebug() << "AppManager::SetAppState(): IDLE";
        // Разрыв соединения и т.д.
        break;
    case AppState::STARTED:
        qDebug() << "AppManager::SetAppState(): STARTED";
        // Запрос на подключение
        emit signalMakeConnection();
        break;
    case AppState::CONNECTED:
        qDebug() << "AppManager::SetAppState(): CONNECTED";
        if(AuthMessage) { // Если AuthMessage нет, запрашиваем
            emit signalSendUniterMessage(AuthMessage);
            break;
        }
        emit signalFindAuthData();
        qDebug() << "AppManager::SetAppState(): emited signalFindAuthData";
        break;
    case AppState::AUTHENTIFICATED:
        qDebug() << "AppManager::SetAppState(): AUTHENTIFICATED";
        emit signalAuthed(true); // Переключение на рабочий виджет
        // Запуск инициализации БД
        {
            QCryptographicHash hash(QCryptographicHash::Sha256);
            if (User) {
                QString userName = User->name + User->surname + User->patronymic;
                hash.addData(userName.toUtf8());
            }
            emit signalLoadResources(hash.result());
        }
        break;
    case AppState::DBLOADED:
        qDebug() << "AppManager::SetAppState(): DBLOADED";
        // Вызов менеджера конфигурации для загрузки ресурсов
        emit signalConfigProc(User);
        break;
    case AppState::READY:
        qDebug() << "AppManager::SetAppState(): READY";
        emit signalPollMessages(); // Запрос на POLL сообщений
        break;
    case AppState::SHUTDOWN:
        qDebug() << "AppManager::SetAppState(): SHUTDOWN";
        // TODO: сохранение настроек и буферов
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
        qDebug() << "AppManager::SetNetState(): OFFLINE";
        emit signalDisconnected(); // Блокировка UI (виджет переподключения)
        break;
    case NetState::ONLINE:
        qDebug() << "AppManager::SetNetState(): ONLINE";

        // Если возникло при запуске, просто переходим в Connected
        if(AState == AppState::STARTED) {
            SetAppState(AppState::CONNECTED);
        }
        // else: уже находимся в каком-то состоянии, просто разблокируем UI

        emit signalConnected();
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

                    qDebug() << "AppManager::onRecvUniterMessage(): got success auth message";
                    SetAppState(AppState::AUTHENTIFICATED);
                    return;
                }
                else if(Message->error == messages::ErrorCode::BAD_REQUEST) {

                    qDebug() << "AppManager::onRecvUniterMessage(): got bad request auth message";
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

                qDebug() << "AppManager::onSendUniterMessage: sending auth request";
                emit signalSendUniterMessage(AuthMessage);
            }
        }

    }

}

// Сигналы от сетевого класса
void AppManager::onConnected() {

    qDebug() << "AppManager::onConnected()";

    SetNetState(NetState::ONLINE);
}

void AppManager::onDisconnected() {

    qDebug() << "AppManager::onDisonnected()";

    SetNetState(NetState::OFFLINE);
}

void AppManager::onResourcesLoaded() {

    qDebug() << "AppManager::onResourcesLoaded()";

    // Переход из AUTHENTICATED в DBLOADED после инициализации БД
    if(AState == AppState::AUTHENTIFICATED) {
        SetAppState(AppState::DBLOADED);
    }
}

void AppManager::onConfigured() {

    qDebug() << "AppManager::onConfigured()";

    // Переход из DBLOADED в READY после завершения работы ConfigManager
    if(AState == AppState::DBLOADED) {
        SetAppState(AppState::READY);
    }
}


void AppManager::onShutDown() {
    // TODO: выполнить сохранение
    // TODO: на выход регестрируем слот shutdown
}


} // namespace uniter::managers
