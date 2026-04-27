
#include "datamanager.h"
#include "sqlitedatabase.h"
#include "../../common/appfuncs.h"
#include "../contract/unitermessage.h"
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <optional>

namespace uniter::data {

namespace {

QString safeUserHash(const QByteArray& userhash)
{
    QString value = QString::fromUtf8(userhash);
    if (value.isEmpty()) {
        value = QStringLiteral("default");
    }

    value.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("_"));
    return value;
}

} // namespace

DataManager* DataManager::instance()
{
    static DataManager instance;
    return &instance;
}

DataManager::DataManager()
    : QObject(nullptr)
    , state(DBState::IDLE)
{
}

void DataManager::setState(DBState newState)
{
    state = newState;
}

QString DataManager::resolveDatabasePath(const QByteArray& userhash) const
{
    QString root = qEnvironmentVariable("TEMP_DIR");
    if (root.isEmpty()) {
        root = common::appfuncs::get_env_data().temp_dir;
    }

    QDir dir(root);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    if (!dir.exists(QStringLiteral("database"))) {
        dir.mkpath(QStringLiteral("database"));
    }

    dir.cd(QStringLiteral("database"));
    return dir.filePath(QStringLiteral("uniter_%1.sqlite").arg(safeUserHash(userhash)));
}

bool DataManager::initializeExecutor(database::IResExecutor& executor)
{
    auto result = executor.Initialize(*db_);
    if (!result.success) {
        qWarning() << "DataManager: executor initialization failed:"
                   << QString::fromStdString(result.message);
        return false;
    }

    result = executor.ApplyMigrations(*db_);
    if (!result.success) {
        qWarning() << "DataManager: executor migration failed:"
                   << QString::fromStdString(result.message);
        return false;
    }

    result = executor.Verify(*db_);
    if (!result.success) {
        qWarning() << "DataManager: executor verification failed:"
                   << QString::fromStdString(result.message);
        return false;
    }

    return true;
}

bool DataManager::initializeDatabase(const QByteArray& userhash)
{
    setState(DBState::LOADING);
    userHash_ = userhash;
    databasePath_ = resolveDatabasePath(userHash_);

    db_ = std::make_unique<SqliteDataBase>();
    db_->Open(databasePath_.toStdString());
    db_->SetUserContext(QString::fromUtf8(userHash_).toStdString());

    if (!initializeExecutor(commonExecutor_)) {
        setState(DBState::ERROR);
        db_->Close();
        db_.reset();
        return false;
    }

    if (!initializeExecutor(managerExecutor_)) {
        setState(DBState::ERROR);
        db_->Close();
        db_.reset();
        return false;
    }

    // TODO: initialize materials executor.
    // initializeExecutor(materialsExecutor_);
    // TODO: initialize design executor.
    // initializeExecutor(designExecutor_);
    // TODO: initialize purchases executor.
    // initializeExecutor(purchasesExecutor_);
    // TODO: initialize instances executor.
    // initializeExecutor(instancesExecutor_);
    // TODO: initialize pdm executor.
    // initializeExecutor(pdmExecutor_);
    // TODO: initialize documents executor.
    // initializeExecutor(documentsExecutor_);
    // TODO: initialize generative production/integration executors.
    // initializeExecutor(productionExecutor_);
    // initializeExecutor(integrationExecutor_);

    setState(DBState::LOADED);
    qDebug() << "DataManager: database loaded:" << databasePath_;
    return true;
}

bool DataManager::clearExecutorData(database::IResExecutor& executor)
{
    const auto result = executor.ClearData(*db_);
    if (!result.success) {
        qWarning() << "DataManager: executor clear failed:"
                   << QString::fromStdString(result.message);
        return false;
    }

    return true;
}

void DataManager::onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message)
{
    if (!message) {
        qWarning() << "DataManager::onRecvUniterMessage(): null message";
        return;
    }
    if (!db_ || state != DBState::LOADED) {
        qWarning() << "DataManager::onRecvUniterMessage(): database is not loaded";
        return;
    }
    if (message->crudact == contract::CrudAction::NOTCRUD) {
        qWarning() << "DataManager::onRecvUniterMessage(): message is not CRUD";
        return;
    }

    const auto result = routeManagerMessage(*message);
    if (!result.has_value()) {
        qWarning() << "DataManager::onRecvUniterMessage(): unsupported subsystem"
                   << static_cast<int>(message->subsystem);
        return;
    }
    if (!result->success) {
        qWarning() << "DataManager::onRecvUniterMessage(): executor failed:"
                   << QString::fromStdString(result->message);
    }
}

void DataManager::onStartLoadResources(QByteArray userhash)
{
    if (initializeDatabase(userhash)) {
        emit signalResourcesLoaded();
    }
}

void DataManager::onClearResources() {
    if (db_) {
        db_->Close();
        db_.reset();
    }

    userHash_.clear();
    databasePath_.clear();
    setState(DBState::IDLE);
}

void DataManager::onClearDatabase()
{
    if (!db_ || state != DBState::LOADED) {
        qWarning() << "DataManager::onClearDatabase(): database is not loaded";
        emit signalDatabaseCleared();
        return;
    }

    bool ok = clearExecutorData(managerExecutor_);

    // TODO: clear materials executor data.
    // ok = ok && clearExecutorData(materialsExecutor_);
    // TODO: clear design executor data.
    // ok = ok && clearExecutorData(designExecutor_);
    // TODO: clear purchases executor data.
    // ok = ok && clearExecutorData(purchasesExecutor_);
    // TODO: clear instances executor data.
    // ok = ok && clearExecutorData(instancesExecutor_);
    // TODO: clear pdm executor data.
    // ok = ok && clearExecutorData(pdmExecutor_);
    // TODO: clear documents executor data.
    // ok = ok && clearExecutorData(documentsExecutor_);
    // TODO: clear generative production/integration executor data.
    // ok = ok && clearExecutorData(productionExecutor_);
    // ok = ok && clearExecutorData(integrationExecutor_);

    if (!ok) {
        setState(DBState::ERROR);
        return;
    }

    emit signalDatabaseCleared();
}

void DataManager::onSubsystemGenerate(contract::Subsystem subsystem,
                                      contract::GenSubsystem genType,
                                      std::optional<uint64_t> genId,
                                      bool created)
{
    (void)subsystem;
    (void)genType;
    (void)genId;
    (void)created;
    // TODO: route generated subsystem lifecycle when generative executors exist.
}

std::optional<database::ExecutorResult> DataManager::routeManagerMessage(const contract::UniterMessage& message)
{
    if (message.subsystem != contract::Subsystem::MANAGER) {
        // TODO: route to materials executor.
        // return materialsExecutor_.ApplyMessage(*db_, message);
        // TODO: route to design executor.
        // return designExecutor_.ApplyMessage(*db_, message);
        // TODO: route to purchases executor.
        // return purchasesExecutor_.ApplyMessage(*db_, message);
        // TODO: route to instances executor.
        // return instancesExecutor_.ApplyMessage(*db_, message);
        // TODO: route to pdm executor.
        // return pdmExecutor_.ApplyMessage(*db_, message);
        // TODO: route to documents executor.
        // return documentsExecutor_.ApplyMessage(*db_, message);
        // TODO: route to generative production/integration executors.
        // return productionExecutor_.ApplyMessage(*db_, message);
        // return integrationExecutor_.ApplyMessage(*db_, message);
        return std::nullopt;
    }

    switch (message.crudact) {
    case contract::CrudAction::CREATE:
        return managerExecutor_.Create(*db_, message);
    case contract::CrudAction::READ:
        return managerExecutor_.Read(*db_, database::ResourceKey{
            message.subsystem,
            message.GenSubsystem,
            message.GenSubsystemId == 0 ? std::optional<uint64_t>{} : std::optional<uint64_t>{message.GenSubsystemId},
            message.resource ? message.resource->resource_type : contract::ResourceType::DEFAULT,
            message.resource ? std::optional<uint64_t>{message.resource->id} : std::optional<uint64_t>{}
        });
    case contract::CrudAction::UPDATE:
        return managerExecutor_.Update(*db_, message);
    case contract::CrudAction::DELETE:
        return managerExecutor_.Delete(*db_, message);
    case contract::CrudAction::NOTCRUD:
    default:
        return database::ExecutorResult::Fail(database::ExecutorStatusCode::InvalidMessage,
                                             "Message is not a CRUD request");
    }
}

} // namespace uniter::data
