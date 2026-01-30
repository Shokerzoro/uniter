
#include "widgets_static/mainwindow.h"
#include "database/datamanager.h"
#include "managers/appmanager.h"
#include "network/mocknetwork.h"
#include "../common/appfuncs.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QThread>

using namespace uniter;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Проверка уникальности приложения
    if(!common::appfuncs::is_single_instance()) return 0;
    common::appfuncs::embed_main_exe_core_data();

    // Задаем переменные окружения
    common::appfuncs::AppEnviroment AEnviroment = common::appfuncs::get_env_data();
    common::appfuncs::set_env(AEnviroment);
    common::appfuncs::open_log(AEnviroment.logfile);

    // Создаем слои - основные компоненты
    staticwdg::MainWidget MWindow;
    data::DataManager DManager;
    managers::AppManager AManager;
    net::MockNetManager MNManager;


    // === 1. Маршрутизация сообщений (всё, что гоняет UniterMessage) ===

    // MainWidget → AppManager (только вверх, от UI к менеджеру)
    QObject::connect(&MWindow,  &staticwdg::MainWidget::signalSendUniterMessage,
                     &AManager, &managers::AppManager::onSendUniterMessage);

    // AppManager ↔ DataManager
    QObject::connect(&DManager, &data::DataManager::signalSendUniterMessage,
                     &AManager, &managers::AppManager::onSendUniterMessage);
    QObject::connect(&AManager, &managers::AppManager::signalRecvUniterMessage,
                     &DManager, &data::DataManager::onRecvUniterMessage);

    // AppManager ↔ MockNetworkProcessor
    QObject::connect(&AManager,  &managers::AppManager::signalSendUniterMessage,
                     &MNManager, &net::MockNetManager::onSendMessage);
    QObject::connect(&MNManager, &net::MockNetManager::signalRecvMessage,
                     &AManager,  &managers::AppManager::onRecvUniterMessage);

    // === 2. Подписки/ресурсы (UI ↔ DataManager) ===

    QObject::connect(&MWindow, &staticwdg::MainWidget::signalSubscribeToResourceList,
                     &DManager, &data::DataManager::onSubscribeToResourceList);
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalSubscribeToResourceTree,
                     &DManager, &data::DataManager::onSubscribeToResourceTree);
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalSubscribeToResource,
                     &DManager, &data::DataManager::onSubscribeToResource);
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalGetResource,
                     &DManager, &data::DataManager::onGetResource);

    // === 3. Управление сетевым состоянием (connect / disconnect) ===

    // Network → AppManager
    QObject::connect(&MNManager, &net::MockNetManager::signalConnected,
                     &AManager,  &managers::AppManager::onConnected);
    QObject::connect(&MNManager, &net::MockNetManager::signalDisconnected,
                     &AManager,  &managers::AppManager::onDisconnected);

    // AppManager → MainWidget (отражение состояния в UI)
    QObject::connect(&AManager, &managers::AppManager::signalConnected,
                     &MWindow,  &staticwdg::MainWidget::onConnected);
    QObject::connect(&AManager, &managers::AppManager::signalDisconnected,
                     &MWindow,  &staticwdg::MainWidget::onDisconnected);

    // === 4. Управление запуском/инициализацией (ресурсы, БД) ===

    // DataManager → AppManager (ресурсы готовы)
    QObject::connect(&DManager, &data::DataManager::signalResourcesLoaded,
                     &AManager, &managers::AppManager::onResourcesLoaded);

    // AppManager → DataManager (старт загрузки ресурсов)
    QObject::connect(&AManager, &managers::AppManager::signalLoadResources,
                     &DManager, &data::DataManager::onStartLoadResources);

    // === 5. Управление аутентификацией и UI ===

    // AppManager → MainWidget (результат аутентификации)
    QObject::connect(&AManager, &managers::AppManager::signalAuthed,
                     &MWindow,  &staticwdg::MainWidget::onAuthed);

    // AppManager → MainWidget (поиск локальных данных аутентификации)
    QObject::connect(&AManager, &managers::AppManager::signalFindAuthData,
                     &MWindow,  &staticwdg::MainWidget::onFindAuthData);

    // === 6. Управляющие сетевым подключением (MakeConnect) ===

    // UI → AppManager (пользователь нажал "подключиться")
    QObject::connect(&MWindow,  &staticwdg::MainWidget::signalMakeConnect,
                     &AManager, &managers::AppManager::signalMakeConnection);

    // AppManager → Network (команда установить соединение)
    QObject::connect(&AManager,  &managers::AppManager::signalMakeConnection,
                     &MNManager, &net::MockNetManager::onMakeConnection);



    // Запуск FSM приложения и подсвечиваем виджеты
    AManager.start_run();
    MWindow.show();

    return app.exec();
}
