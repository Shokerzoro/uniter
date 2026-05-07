
#include "widgets_static/mainwindow.h"
#include "data/datamanager.h"
#include "control/appmanager.h"
#include "control/configmanager.h"
#include "network/serverconnector.h"
#include "network/kafkaconnector.h"
#include "network/miniconnector.h"
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
    auto* dataManager    = data::DataManager::instance();
    auto* appManager     = control::AppManager::instance();
    auto* configManager  = control::ConfigManager::instance();
    auto* serverConnector = net::ServerConnector::instance();
    auto* kafkaConnector  = net::KafkaConnector::instance();
    auto* minioConnector  = net::MinIOConnector::instance();

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

    // ServerConnector → AppManager (входящие RESPONSE/ERROR/SUCCESS)
    QObject::connect(serverConnector, &net::ServerConnector::signalRecvMessage,
                     appManager, &control::AppManager::onRecvUniterMessage);

    // KafkaConnector → AppManager (входящие NOTIFICATION broadcast-очереди)
    QObject::connect(kafkaConnector, &net::KafkaConnector::signalRecvMessage,
                     appManager, &control::AppManager::onRecvUniterMessage);

    // MinIOConnector → AppManager (ответы GET/PUT файлов)
    QObject::connect(minioConnector, &net::MinIOConnector::signalRecvMessage,
                     appManager, &control::AppManager::onRecvUniterMessage);

    // AppManager → DataManager (CRUD в READY, как правило от KafkaConnector)
    QObject::connect(appManager, &control::AppManager::signalRecvUniterMessage,
                     dataManager, &data::DataManager::onRecvUniterMessage);

    // AppManager → FileManager (MINIO-протокольные сообщения в READY)
    // TODO: FileManager пока не реализован — коннект будет добавлен,
    // когда появится data::FileManager::onForwardUniterMessage.
    // QObject::connect(appManager, &control::AppManager::signalForwardToFileManager,
    //                  fileManager, &data::FileManager::onForwardUniterMessage);

    // AppManager → ServerConnector (AUTH / GET_KAFKA_CREDENTIALS / FULL_SYNC
    // / GET_MINIO_PRESIGNED_URL / CRUD в READY / UPDATE_*)
    QObject::connect(appManager, &control::AppManager::signalSendToServer,
                     serverConnector, &net::ServerConnector::onSendMessage);

    // AppManager → MinIOConnector (GET_MINIO_FILE, PUT_MINIO_FILE)
    QObject::connect(appManager, &control::AppManager::signalSendToMinio,
                     minioConnector, &net::MinIOConnector::onSendMessage);

    // === 2. Управление сетевым состоянием ===

    // ServerConnector → AppManager
    QObject::connect(serverConnector, &net::ServerConnector::signalConnectionUpdated,
                     appManager, &control::AppManager::onConnectionUpdated);

    // AppManager → ServerConnector
    QObject::connect(appManager, &control::AppManager::signalMakeConnection,
                     serverConnector, &net::ServerConnector::onMakeConnection);

    // AppManager → UI (отражение состояния)
    QObject::connect(appManager, &control::AppManager::signalConnectionUpdated,
                     &MWindow, &staticwdg::MainWidget::onConnectionUpdated);

    // === 2.1 KafkaConnector (KCONNECTOR / offset / подписка) ===

    // AppManager → KafkaConnector: инициализация под пользователя (войдя в KCONNECTOR)
    QObject::connect(appManager, &control::AppManager::signalInitKafkaConnector,
                     kafkaConnector, &net::KafkaConnector::onInitConnection);

    // KafkaConnector → AppManager: offset получен → переход KCONNECTOR → KAFKA
    QObject::connect(kafkaConnector, &net::KafkaConnector::signalOffsetReady,
                     appManager, &control::AppManager::onKafkaOffsetReceived);

    // AppManager → KafkaConnector: подписка на broadcast-топик (войдя в READY)
    QObject::connect(appManager, &control::AppManager::signalSubscribeKafka,
                     kafkaConnector, &net::KafkaConnector::onSubscribeKafka);

    // ==========================================================================
    // === 2.2 ВРЕМЕННЫЕ СВЯЗИ СЕТЕВЫХ ЗАГЛУШЕК МЕЖДУ СОБОЙ ===
    // ==========================================================================
    // Эти коннекты эмулируют работу реального бэкенда: сервер публикует
    // NOTIFICATION в Kafka; сервер общается с MinIO для получения presigned URL.
    // При переходе на реальную сеть — УДАЛИТЬ: KafkaConnector будет получать
    // сообщения напрямую от брокера; ServerConnector будет получать presigned
    // URL от настоящего сервера без участия клиентского MinIOConnector.
    // ==========================================================================

    // TEMP: ServerConnector → KafkaConnector
    // Сервер имитирует broadcast: после подтверждения CUD рассылает
    // NOTIFICATION "через Kafka".
    QObject::connect(serverConnector, &net::ServerConnector::signalEmitKafkaNotification,
                     kafkaConnector, &net::KafkaConnector::onServerNotification);

    // TEMP: ServerConnector → MinIOConnector
    // Сервер при получении GET_MINIO_PRESIGNED_URL делегирует локальному
    // MinIOConnector (в реальной системе — внешний MinIO, не клиент).
    QObject::connect(serverConnector, &net::ServerConnector::signalRequestMinioPresignedUrl,
                     minioConnector, &net::MinIOConnector::onRequestPresignedUrl);

    // TEMP: MinIOConnector → ServerConnector
    // Presigned URL готов → сервер оборачивает его в RESPONSE клиенту.
    QObject::connect(minioConnector, &net::MinIOConnector::signalPresignedUrlReady,
                     serverConnector, &net::ServerConnector::onMinioPresignedUrlReady);

    // ==========================================================================

    // === 3. Управление ресурсами и БД ===

    // AppManager → DataManager
    QObject::connect(appManager, &control::AppManager::signalLoadResources,
                     dataManager, &data::DataManager::onStartLoadResources);

    // DataManager → AppManager
    QObject::connect(dataManager, &data::DataManager::signalResourcesLoaded,
                     appManager, &control::AppManager::onResourcesLoaded);

    QObject::connect(appManager, &control::AppManager::signalClearResources,
                     dataManager, &data::DataManager::onClearResources);

    // DBCLEAR: AppManager → DataManager и обратно
    QObject::connect(appManager, &control::AppManager::signalClearDatabase,
                     dataManager, &data::DataManager::onClearDatabase);

    QObject::connect(dataManager, &data::DataManager::signalDatabaseCleared,
                     appManager, &control::AppManager::onDatabaseCleared);


    // === 4. Конфигурация ===

    // ConfigManager → AppManager
    QObject::connect(configManager, &control::ConfigManager::signalConfigured,
                     appManager, &control::AppManager::onConfigured);

    // AppManager → ConfigManager
    QObject::connect(appManager, &control::AppManager::signalConfigProc,
                     configManager, &control::ConfigManager::onConfigProc);

    QObject::connect(appManager, &control::AppManager::signalClearResources,
                     configManager, &control::ConfigManager::onClearResources);


    // === 5. Аутентификация ===

    // AppManager → UI
    QObject::connect(appManager, &control::AppManager::signalAuthed,
                     &MWindow, &staticwdg::MainWidget::onAuthed);

    QObject::connect(appManager, &control::AppManager::signalFindAuthData,
                     &MWindow, &staticwdg::MainWidget::onFindAuthData);

    QObject::connect(appManager, &control::AppManager::signalLoggedOut,
                     &MWindow, &staticwdg::MainWidget::onLoggedOut);



    // ==========================================================================
    // ЗАПУСК ПРИЛОЖЕНИЯ (после установки всех связей)
    // ==========================================================================

    appManager->start_run();

    return app.exec();
}
