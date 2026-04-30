
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

    // Checking the uniqueness of the application
    if(!common::appfuncs::is_single_instance()) return 0;
    common::appfuncs::embed_main_exe_core_data();

    // Setting environment variables
    common::appfuncs::AppEnviroment AEnviroment = common::appfuncs::get_env_data();
    common::appfuncs::set_env(AEnviroment);
    common::appfuncs::open_log(AEnviroment.logfile);

    // ========== INITIALIZING SINGLETONS ==========
    auto* dataManager    = data::DataManager::instance();
    auto* appManager     = control::AppManager::instance();
    auto* configManager  = control::ConfigManager::instance();
    auto* serverConnector = net::ServerConnector::instance();
    auto* kafkaConnector  = net::KafkaConnector::instance();
    auto* minioConnector  = net::MinIOConnector::instance();

    // ========== CREATE UI ==========
    staticwdg::MainWidget MWindow;

    qDebug() << "MainWindow created, visible:" << MWindow.isVisible();
    MWindow.show();
    qDebug() << "MWindow shown, visible:" << MWindow.isVisible();
    qDebug() << "Window handle:" << MWindow.windowHandle();


    // ==========================================================================
    // STATIC SIGNAL/SLOT COMMUNICATIONS (all in one place for checking)
    // ==========================================================================

    // === 1. UniterMessage routing ===

    // ServerConnector → AppManager (incoming RESPONSE/ERROR/SUCCESS)
    QObject::connect(serverConnector, &net::ServerConnector::signalRecvMessage,
                     appManager, &control::AppManager::onRecvUniterMessage);

    // KafkaConnector → AppManager (incoming NOTIFICATION broadcast queues)
    QObject::connect(kafkaConnector, &net::KafkaConnector::signalRecvMessage,
                     appManager, &control::AppManager::onRecvUniterMessage);

    // MinIOConnector → AppManager (GET/PUT file responses)
    QObject::connect(minioConnector, &net::MinIOConnector::signalRecvMessage,
                     appManager, &control::AppManager::onRecvUniterMessage);

    // AppManager → DataManager (CRUD in READY, usually from KafkaConnector)
    QObject::connect(appManager, &control::AppManager::signalRecvUniterMessage,
                     dataManager, &data::DataManager::onRecvUniterMessage);

    // AppManager → FileManager (MINIO protocol messages in READY)
    // TODO: FileManager has not yet been implemented - the connection will be added,
    // when data::FileManager::onForwardUniterMessage appears.
    // QObject::connect(appManager, &control::AppManager::signalForwardToFileManager,
    //                  fileManager, &data::FileManager::onForwardUniterMessage);

    // AppManager → ServerConnector (AUTH / GET_KAFKA_CREDENTIALS / FULL_SYNC
    // / GET_MINIO_PRESIGNED_URL / CRUD in READY / UPDATE_*)
    QObject::connect(appManager, &control::AppManager::signalSendToServer,
                     serverConnector, &net::ServerConnector::onSendMessage);

    // AppManager → MinIOConnector (GET_MINIO_FILE, PUT_MINIO_FILE)
    QObject::connect(appManager, &control::AppManager::signalSendToMinio,
                     minioConnector, &net::MinIOConnector::onSendMessage);

    // === 2. Network state management ===

    // ServerConnector → AppManager
    QObject::connect(serverConnector, &net::ServerConnector::signalConnectionUpdated,
                     appManager, &control::AppManager::onConnectionUpdated);

    // AppManager → ServerConnector
    QObject::connect(appManager, &control::AppManager::signalMakeConnection,
                     serverConnector, &net::ServerConnector::onMakeConnection);

    // AppManager → UI (state reflection)
    QObject::connect(appManager, &control::AppManager::signalConnectionUpdated,
                     &MWindow, &staticwdg::MainWidget::onConnectionUpdated);

    // === 2.1 KafkaConnector(KCONNECTOR/offset/subscription) ===

    // AppManager → KafkaConnector: initialization for the user (by logging into KCONNECTOR)
    QObject::connect(appManager, &control::AppManager::signalInitKafkaConnector,
                     kafkaConnector, &net::KafkaConnector::onInitConnection);

    // KafkaConnector → AppManager: offset received → transition KCONNECTOR → KAFKA
    QObject::connect(kafkaConnector, &net::KafkaConnector::signalOffsetReady,
                     appManager, &control::AppManager::onKafkaOffsetReceived);

    // AppManager → KafkaConnector: subscription to a broadcast topic (by logging into READY)
    QObject::connect(appManager, &control::AppManager::signalSubscribeKafka,
                     kafkaConnector, &net::KafkaConnector::onSubscribeKafka);

    QObject::connect(appManager, &control::AppManager::signalForgetKafkaOffset,
                     kafkaConnector, &net::KafkaConnector::onForgetOffset);

    // ==========================================================================
    // === 2.2 TEMPORARY CONNECTIONS OF NETWORK STUBS BETWEEN THEM ===
    // ==========================================================================
    // These connections emulate the work of a real backend: the server publishes
    // NOTIFICATION in Kafka; the server communicates with MinIO to obtain the presigned URL.
    // When moving to a real network - DELETE: KafkaConnector will receive
    // messages directly from the broker; ServerConnector will receive presigned
    // URL from the real server without the participation of the client MinIOConnector.
    // ==========================================================================

    // TEMP: ServerConnector → KafkaConnector
    // The server simulates a broadcast: after confirmation, the CUD sends out
    // NOTIFICATION "via Kafka".
    QObject::connect(serverConnector, &net::ServerConnector::signalEmitKafkaNotification,
                     kafkaConnector, &net::KafkaConnector::onServerNotification);

    // TEMP: ServerConnector → MinIOConnector
    // The server, upon receiving GET_MINIO_PRESIGNED_URL, delegates to the local
    // MinIOConnector (in a real system - external MinIO, not a client).
    QObject::connect(serverConnector, &net::ServerConnector::signalRequestMinioPresignedUrl,
                     minioConnector, &net::MinIOConnector::onRequestPresignedUrl);

    // TEMP: MinIOConnector → ServerConnector
    // Presigned URL is ready → the server wraps it in RESPONSE to the client.
    QObject::connect(minioConnector, &net::MinIOConnector::signalPresignedUrlReady,
                     serverConnector, &net::ServerConnector::onMinioPresignedUrlReady);

    // ==========================================================================

    // === 3. Resource and database management ===

    // AppManager → DataManager
    QObject::connect(appManager, &control::AppManager::signalLoadResources,
                     dataManager, &data::DataManager::onInitDatabase);

    // DataManager → AppManager
    QObject::connect(dataManager, &data::DataManager::signalResourcesLoaded,
                     appManager, &control::AppManager::onResourcesLoaded);

    QObject::connect(appManager, &control::AppManager::signalClearResources,
                     dataManager, &data::DataManager::onResetDatabase);

    // DBCLEAR: AppManager → DataManager and back
    QObject::connect(appManager, &control::AppManager::signalClearDatabase,
                     dataManager, &data::DataManager::onClearResources);

    QObject::connect(dataManager, &data::DataManager::signalResourcesCleared,
                     appManager, &control::AppManager::onDatabaseCleared);


    // === 4. Configuration ===

    // ConfigManager → AppManager
    QObject::connect(configManager, &control::ConfigManager::signalConfigured,
                     appManager, &control::AppManager::onConfigured);

    // AppManager → ConfigManager
    QObject::connect(appManager, &control::AppManager::signalConfigProc,
                     configManager, &control::ConfigManager::onConfigProc);

    QObject::connect(appManager, &control::AppManager::signalClearResources,
                     configManager, &control::ConfigManager::onClearResources);

    QObject::connect(configManager, &control::ConfigManager::signalSubsystemAdded,
                     dataManager, &data::DataManager::onSubsystemGenerate);


    // === 5. Authentication ===

    // AppManager → UI
    QObject::connect(appManager, &control::AppManager::signalAuthed,
                     &MWindow, &staticwdg::MainWidget::onAuthed);

    QObject::connect(appManager, &control::AppManager::signalFindAuthData,
                     &MWindow, &staticwdg::MainWidget::onFindAuthData);

    QObject::connect(appManager, &control::AppManager::signalLoggedOut,
                     &MWindow, &staticwdg::MainWidget::onLoggedOut);



    // ==========================================================================
    // LAUNCHING THE APPLICATION (after all connections have been established)
    // ==========================================================================

    appManager->start_run();

    return app.exec();
}
