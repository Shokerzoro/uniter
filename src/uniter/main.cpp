
#include "widgets_static/mainwindow.h"
#include "data/datamanager.h"
#include "managers/appmanager.h"
#include "managers/configmanager.h"
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

    // ========== ИНИЦИАЛИЗАЦИЯ СИНГЛТОНОВ ==========
    auto* dataManager = data::DataManager::instance();
    auto* appManager = managers::AppManager::instance();
    auto* configManager = managers::ConfigManager::instance();
    auto* netManager = net::MockNetManager::instance();

    // ========== СОЗДАНИЕ UI ==========
    staticwdg::MainWidget MWindow;


    // ==========================================================================
    // СТАТИЧЕСКИЕ СВЯЗИ СИГНАЛОВ/СЛОТОВ (все в одном месте для проверки)
    // ==========================================================================

    // === 1. Маршрутизация UniterMessage ===

    // Network → AppManager (входящие сообщения из сети)
    QObject::connect(netManager, &net::MockNetManager::signalRecvMessage,
                     appManager, &managers::AppManager::onRecvUniterMessage);

    // AppManager → DataManager (пересылка входящих сообщений)
    QObject::connect(appManager, &managers::AppManager::signalRecvUniterMessage,
                     dataManager, &data::DataManager::onRecvUniterMessage);

    // AppManager → Network (исходящие сообщения в сеть)
    QObject::connect(appManager, &managers::AppManager::signalSendUniterMessage,
                     netManager, &net::MockNetManager::onSendMessage);

    // MainWidget → AppManager (исходящие сообщения от UI)
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalSendUniterMessage,
                     appManager, &managers::AppManager::onSendUniterMessage);


    // === 2. Управление сетевым состоянием ===

    // Network → AppManager
    QObject::connect(netManager, &net::MockNetManager::signalConnected,
                     appManager, &managers::AppManager::onConnected);
    QObject::connect(netManager, &net::MockNetManager::signalDisconnected,
                     appManager, &managers::AppManager::onDisconnected);

    // AppManager → Network
    QObject::connect(appManager, &managers::AppManager::signalMakeConnection,
                     netManager, &net::MockNetManager::onMakeConnection);

    // AppManager → UI (отражение состояния)
    QObject::connect(appManager, &managers::AppManager::signalConnected,
                     &MWindow, &staticwdg::MainWidget::onConnected);
    QObject::connect(appManager, &managers::AppManager::signalDisconnected,
                     &MWindow, &staticwdg::MainWidget::onDisconnected);

    // UI → AppManager (команда подключиться)
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalMakeConnect,
                     appManager, &managers::AppManager::signalMakeConnection);


    // === 3. Управление ресурсами и БД ===

    // AppManager → DataManager
    QObject::connect(appManager, &managers::AppManager::signalLoadResources,
                     dataManager, &data::DataManager::onStartLoadResources);

    // DataManager → AppManager
    QObject::connect(dataManager, &data::DataManager::signalResourcesLoaded,
                     appManager, &managers::AppManager::onResourcesLoaded);


    // === 4. Конфигурация ===

    // ConfigManager → AppManager
    QObject::connect(configManager, &managers::ConfigManager::signalConfigured,
                     appManager, &managers::AppManager::onConfigured);

    // AppManager → ConfigManager
    QObject::connect(appManager, &managers::AppManager::signalConfigProc,
                     configManager, &managers::ConfigManager::onConfigProc);


    // === 5. Аутентификация ===

    // AppManager → UI
    QObject::connect(appManager, &managers::AppManager::signalAuthed,
                     &MWindow, &staticwdg::MainWidget::onAuthed);
    QObject::connect(appManager, &managers::AppManager::signalFindAuthData,
                     &MWindow, &staticwdg::MainWidget::onFindAuthData);


    // === 6. Подписки на ресурсы (UI и динамические виджеты → DataManager) ===

    QObject::connect(&MWindow, &staticwdg::MainWidget::signalSubscribeToResourceList,
                     dataManager, &data::DataManager::onSubscribeToResourceList);
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalSubscribeToResourceTree,
                     dataManager, &data::DataManager::onSubscribeToResourceTree);
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalSubscribeToResource,
                     dataManager, &data::DataManager::onSubscribeToResource);
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalGetResource,
                     dataManager, &data::DataManager::onGetResource);


    // ==========================================================================
    // ЗАПУСК ПРИЛОЖЕНИЯ (после установки всех связей)
    // ==========================================================================

    appManager->start_run();
    MWindow.show();

    return app.exec();
}
