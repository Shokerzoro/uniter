
#include "widgets_static/mainwindow.h"
#include "data/datamanager.h"
#include "control/appmanager.h"
#include "control/configmanager.h"
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
    auto* appManager = control::AppManager::instance();
    auto* configManager = control::ConfigManager::instance();
    auto* netManager = net::MockNetManager::instance();

    // ========== СОЗДАНИЕ UI ==========
    staticwdg::MainWidget MWindow;

    qDebug() << "MainWindow created, visible:" << MWindow.isVisible();

    MWindow.show();

    qDebug() << "MWindow shown, visible:" << MWindow.isVisible();
    qDebug() << "Window handle:" << MWindow.windowHandle();


    // ==========================================================================
    // СТАТИЧЕСКИЕ СВЯЗИ СИГНАЛОВ/СЛОТОВ (все в одном месте для проверки)
    // ==========================================================================

    // === 1. Маршрутизация UniterMessage ===

    // Network → AppManager (входящие сообщения из сети)
    QObject::connect(netManager, &net::MockNetManager::signalRecvMessage,
                     appManager, &control::AppManager::onRecvUniterMessage);

    // AppManager → DataManager (пересылка входящих сообщений)
    QObject::connect(appManager, &control::AppManager::signalRecvUniterMessage,
                     dataManager, &data::DataManager::onRecvUniterMessage);

    // AppManager → Network (исходящие сообщения в сеть)
    QObject::connect(appManager, &control::AppManager::signalSendUniterMessage,
                     netManager, &net::MockNetManager::onSendMessage);

    // MainWidget → AppManager (исходящие сообщения от UI)
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalSendUniterMessage,
                     appManager, &control::AppManager::onSendUniterMessage);


    // === 2. Управление сетевым состоянием ===

    // Network → AppManager
    QObject::connect(netManager, &net::MockNetManager::signalConnected,
                     appManager, &control::AppManager::onConnected);
    QObject::connect(netManager, &net::MockNetManager::signalDisconnected,
                     appManager, &control::AppManager::onDisconnected);

    // AppManager → Network
    QObject::connect(appManager, &control::AppManager::signalMakeConnection,
                     netManager, &net::MockNetManager::onMakeConnection);

    // AppManager → UI (отражение состояния)
    QObject::connect(appManager, &control::AppManager::signalConnected,
                     &MWindow, &staticwdg::MainWidget::onConnected);
    QObject::connect(appManager, &control::AppManager::signalDisconnected,
                     &MWindow, &staticwdg::MainWidget::onDisconnected);

    // UI → AppManager (команда подключиться)
    QObject::connect(&MWindow, &staticwdg::MainWidget::signalMakeConnect,
                     appManager, &control::AppManager::signalMakeConnection);


    // === 3. Управление ресурсами и БД ===

    // AppManager → DataManager
    QObject::connect(appManager, &control::AppManager::signalLoadResources,
                     dataManager, &data::DataManager::onStartLoadResources);

    // DataManager → AppManager
    QObject::connect(dataManager, &data::DataManager::signalResourcesLoaded,
                     appManager, &control::AppManager::onResourcesLoaded);


    // === 4. Конфигурация ===

    // ConfigManager → AppManager
    QObject::connect(configManager, &control::ConfigManager::signalConfigured,
                     appManager, &control::AppManager::onConfigured);

    // AppManager → ConfigManager
    QObject::connect(appManager, &control::AppManager::signalConfigProc,
                     configManager, &control::ConfigManager::onConfigProc);


    // === 5. Аутентификация ===

    // AppManager → UI
    QObject::connect(appManager, &control::AppManager::signalAuthed,
                     &MWindow, &staticwdg::MainWidget::onAuthed);
    QObject::connect(appManager, &control::AppManager::signalFindAuthData,
                     &MWindow, &staticwdg::MainWidget::onFindAuthData);



    // ==========================================================================
    // ЗАПУСК ПРИЛОЖЕНИЯ (после установки всех связей)
    // ==========================================================================

    appManager->start_run();
    //MWindow.show();

    return app.exec();
}
