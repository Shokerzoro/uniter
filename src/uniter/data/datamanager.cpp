#include "datamanager.h"
#include "sqlitedatabase.h"
#include "../../common/appfuncs.h"
#include "../database/transaction.h"
#include "../database/common/commonexecutor.h"
//#include "../database/design/designexecutor.h"
//#include "../database/documents/documentsexecutor.h"
//#include "../database/instance/instanceexecutor.h"
//#include "../database/integration/integrationexecutor.h"
#include "../database/manager/managerexecutor.h"
//#include "../database/material/materialexecutor.h"
//#include "../database/pdm/pdmexecutor.h"
//#include "../database/production/productionexecutor.h"
//#include "../database/supply/supplyexecutor.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <algorithm>
#include <utility>

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
    executors_.emplace_back(std::make_unique<database::CommonExecutor>());
    executors_.emplace_back(std::make_unique<database::ManagerExecutor>());
    // executors_.emplace_back(std::make_unique<database::DocumentsExecutor>());
    // executors_.emplace_back(std::make_unique<database::MaterialExecutor>());
    // executors_.emplace_back(std::make_unique<database::InstanceExecutor>());
    // executors_.emplace_back(std::make_unique<database::DesignExecutor>());
    // executors_.emplace_back(std::make_unique<database::PdmExecutor>());
    // executors_.emplace_back(std::make_unique<database::SupplyExecutor>());
    // executors_.emplace_back(std::make_unique<database::ProductionExecutor>());
    // executors_.emplace_back(std::make_unique<database::IntegrationExecutor>());
}

void DataManager::setState(DBState newState)
{
    state = newState;
}

bool DataManager::processEvent(DBEvent event)
{
    if (event == DBEvent::RESET_DATABASE) {
        setState(DBState::IDLE);
        return true;
    }

    switch (state) {
    case DBState::IDLE:
        if (event == DBEvent::INIT_DATABASE) {
            setState(DBState::LOADING);
            return true;
        }
        break;
    case DBState::LOADING:
        if (event == DBEvent::RESOURCES_LOADED) {
            setState(DBState::LOADED);
            return true;
        }
        if (event == DBEvent::FAIL) {
            setState(DBState::ERROR);
            return true;
        }
        break;
    case DBState::LOADED:
        if (event == DBEvent::CLEAR_RESOURCES) {
            setState(DBState::CLEARING);
            return true;
        }
        break;
    case DBState::CLEARING:
        if (event == DBEvent::RESOURCES_CLEARED) {
            setState(DBState::LOADED);
            return true;
        }
        break;
    case DBState::ERROR:
        if (event == DBEvent::CLEAR_RESOURCES) {
            setState(DBState::CLEARING);
            return true;
        }
        break;
    }

    qWarning() << "DataManager::processEvent(): ignored invalid transition"
               << static_cast<int>(state) << static_cast<int>(event);
    return false;
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

database::IResExecutor* DataManager::findExecutor(contract::Subsystem subsystem)
{
    const auto it = std::find_if(executors_.begin(), executors_.end(),
                                 [subsystem](const auto& executor) {
                                     return executor && executor->Subsystem() == subsystem;
                                 });
    return it == executors_.end() ? nullptr : it->get();
}

const database::IResExecutor* DataManager::findExecutor(contract::Subsystem subsystem) const
{
    const auto it = std::find_if(executors_.begin(), executors_.end(),
                                 [subsystem](const auto& executor) {
                                     return executor && executor->Subsystem() == subsystem;
                                 });
    return it == executors_.end() ? nullptr : it->get();
}

bool DataManager::initializeExecutors()
{
    for (const auto& executor : executors_) {
        const auto result = executor->Initialize(*db_);
        if (!result.success) {
            qWarning() << "DataManager::initializeExecutors(): executor failed:"
                       << static_cast<int>(executor->Subsystem())
                       << QString::fromStdString(result.message);
            return false;
        }
    }
    return true;
}

bool DataManager::verifyExecutors()
{
    for (const auto& executor : executors_) {
        const auto result = executor->Verify(*db_);
        if (!result.success) {
            qWarning() << "DataManager::verifyExecutors(): executor failed:"
                       << static_cast<int>(executor->Subsystem())
                       << QString::fromStdString(result.message);
            return false;
        }
    }
    return true;
}

bool DataManager::dropExecutorStructures()
{
    for (auto it = executors_.rbegin(); it != executors_.rend(); ++it) {
        const auto& executor = *it;
        const auto result = executor->DropStructures(*db_);
        if (!result.success) {
            qWarning() << "DataManager::dropExecutorStructures(): executor failed:"
                       << static_cast<int>(executor->Subsystem())
                       << QString::fromStdString(result.message);
            return false;
        }
    }
    return true;
}

bool DataManager::openAndInitializeDatabase()
{
    db_ = std::make_unique<SqliteDataBase>();
    db_->Open(databasePath_.toStdString());
    db_->SetUserContext(userHash_.toStdString());

    if (!initializeExecutors()) {
        return false;
    }

    if (!verifyExecutors()) {
        return false;
    }

    return true;
}

bool DataManager::dropAndReinitializeDatabase()
{
    if (!db_) {
        db_ = std::make_unique<SqliteDataBase>();
        db_->Open(databasePath_.toStdString());
        db_->SetUserContext(userHash_.toStdString());
    }

    if (!dropExecutorStructures()) {
        return false;
    }

    if (!initializeExecutors()) {
        return false;
    }

    if (!verifyExecutors()) {
        return false;
    }

    return true;
}

void DataManager::failLoading()
{
    processEvent(DBEvent::FAIL);
    emit signalResourcesLoaded(false);
}

void DataManager::subscribeResource(DataAdapter* adapter)
{
    if (!adapter) {
        return;
    }

    unsubscribeResource(adapter);
    const auto key = adapter->resourceKey();
    if (!key.has_value()) {
        return;
    }

    // DataManager stores only guarded non-owning pointers. Adapters stay owned
    // by UI/business objects and unsubscribe themselves on destruction.
    resourceSubscribers_.insert({*key, QPointer<DataAdapter>(adapter)});

    connect(adapter, &QObject::destroyed, this, [this, adapter]() {
        unsubscribeResource(adapter);
    });

    const auto result = readResource(*key);
    if (result && result->success) {
        adapter->initializeData(result->resource);
    }
}

void DataManager::subscribeResourceList(DataAdapter* adapter)
{
    if (!adapter) {
        return;
    }

    unsubscribeResourceList(adapter);

    resourceListSubscribers_.insert({adapter->subsystemKey(), QPointer<DataAdapter>(adapter)});

    connect(adapter, &QObject::destroyed, this, [this, adapter]() {
        unsubscribeResourceList(adapter);
    });

    const auto result = readVector(adapter->subsystemKey());
    if (result && result->success) {
        adapter->initializeData(result->resources);
    }
}

void DataManager::unsubscribeResource(DataAdapter* adapter)
{
    if (!adapter) {
        return;
    }

    for (auto it = resourceSubscribers_.begin(); it != resourceSubscribers_.end();) {
        if (it->second.isNull() || it->second.data() == adapter) {
            it = resourceSubscribers_.erase(it);
        } else {
            ++it;
        }
    }
}

void DataManager::unsubscribeResourceList(DataAdapter* adapter)
{
    if (!adapter) {
        return;
    }

    for (auto it = resourceListSubscribers_.begin(); it != resourceListSubscribers_.end();) {
        if (it->second.isNull() || it->second.data() == adapter) {
            it = resourceListSubscribers_.erase(it);
        } else {
            ++it;
        }
    }
}

void DataManager::onInitDatabase(QByteArray userhash)
{
    if (!processEvent(DBEvent::INIT_DATABASE)) {
        return;
    }

    userHash_ = std::move(userhash);
    databasePath_ = resolveDatabasePath(userHash_);

    if (!openAndInitializeDatabase()) {
        failLoading();
        return;
    }

    processEvent(DBEvent::RESOURCES_LOADED);
    emit signalResourcesLoaded(true);
}

void DataManager::onClearResources()
{
    if (!processEvent(DBEvent::CLEAR_RESOURCES)) {
        return;
    }

    if (db_) {
        db_->Close();
        db_.reset();
    }

    if (!databasePath_.isEmpty()) {
        bool removedAllFiles = true;
        const QString files[] = {
            databasePath_ + QStringLiteral("-wal"),
            databasePath_ + QStringLiteral("-shm"),
            databasePath_
        };
        for (const auto& file : files) {
            if (QFile::exists(file) && !QFile::remove(file)) {
                qWarning() << "DataManager::onClearResources(): failed to remove database file"
                           << file;
                removedAllFiles = false;
                break;
            }
        }
        if (!removedAllFiles) {
            qWarning() << "DataManager::onClearResources(): falling back to dropping database structures";
            if (!dropAndReinitializeDatabase()) {
                setState(DBState::ERROR);
                return;
            }

            processEvent(DBEvent::RESOURCES_CLEARED);
            emit signalResourcesCleared();
            return;
        }
    }

    if (databasePath_.isEmpty()) {
        databasePath_ = resolveDatabasePath(userHash_);
    }

    if (!openAndInitializeDatabase()) {
        qWarning() << "DataManager::onClearResources(): failed to recreate database";
        setState(DBState::ERROR);
        return;
    }

    processEvent(DBEvent::RESOURCES_CLEARED);
    emit signalResourcesCleared();
}

void DataManager::onResetDatabase()
{
    if (db_) {
        db_->Close();
        db_.reset();
    }

    userHash_.clear();
    databasePath_.clear();
    resourceSubscribers_.clear();
    resourceListSubscribers_.clear();
    activeSubsystemContexts_.clear();
    processEvent(DBEvent::RESET_DATABASE);
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

    const auto result = routeMessage(*message);
    if (!result.has_value()) {
        qWarning() << "DataManager::onRecvUniterMessage(): unsupported subsystem"
                   << static_cast<int>(message->subsystem);
        return;
    }
    if (!result->success) {
        qWarning() << "DataManager::onRecvUniterMessage(): executor failed:"
                   << QString::fromStdString(result->message);
        return;
    }

    // Notify observers about changes in resources
    if (message->crudact == contract::CrudAction::CREATE
        || message->crudact == contract::CrudAction::UPDATE
        || message->crudact == contract::CrudAction::DELETE) {
        notifyObservers(*message);
    }
}

void DataManager::onSubsystemGenerate(contract::Subsystem subsystem,
                                      contract::GenSubsystem genType,
                                      std::optional<uint64_t> genId,
                                      bool created)
{
    const auto key = contract::SubsystemKey::Context(subsystem, genType, genId);
    if (created) {
        activeSubsystemContexts_[key] = true;
        return;
    }

    activeSubsystemContexts_.erase(key);

    for (auto it = resourceListSubscribers_.begin(); it != resourceListSubscribers_.end();) {
        if (it->first.subsystem == key.subsystem
            && it->first.genSubsystem == key.genSubsystem
            && it->first.genId == key.genId) {
            it = resourceListSubscribers_.erase(it);
        } else {
            ++it;
        }
    }
}

std::optional<database::ExecutorResult> DataManager::routeMessage(const contract::UniterMessage& message)
{
    if (message.subsystem == contract::Subsystem::MANAGER) {
        auto* executor = dynamic_cast<database::ManagerExecutor*>(findExecutor(contract::Subsystem::MANAGER));
        if (!executor) {
            return database::ExecutorResult::Fail(database::ExecutorStatusCode::InternalError,
                                                 "ManagerExecutor is not registered");
        }

        switch (message.crudact) {
            case contract::CrudAction::CREATE:
                // DataManager routes messages; concrete DB work stays inside the subsystem executor.
                return executor->Create(*db_, message);
            case contract::CrudAction::READ:
                // READ uses the executor API for explicit resource requests only.
                return executor->Read(*db_, contract::ResourceKey::FromMessage(message));
            case contract::CrudAction::UPDATE:
                // DataManager does not inspect or mutate tables directly.
                return executor->Update(*db_, message);
            case contract::CrudAction::DELETE:
                // After successful executor delete, observer adapters are notified below.
                return executor->Delete(*db_, message);
            case contract::CrudAction::NOTCRUD:
            default:
                return database::ExecutorResult::Fail(database::ExecutorStatusCode::InvalidMessage,
                                                     "Message is not a CRUD request");
        }
    }

    // TODO: route non-manager messages to subsystem executors after they are implemented.
    return std::nullopt;
}

std::optional<database::ExecutorResult> DataManager::readResource(const contract::ResourceKey& key)
{
    if (!db_ || state != DBState::LOADED) {
        return std::nullopt;
    }

    if (key.subsystemKey.subsystem == contract::Subsystem::MANAGER) {
        auto* executor = dynamic_cast<database::ManagerExecutor*>(findExecutor(contract::Subsystem::MANAGER));
        if (!executor) {
            return database::ExecutorResult::Fail(database::ExecutorStatusCode::InternalError,
                                                 "ManagerExecutor is not registered");
        }
        return executor->Read(*db_, key);
    }

    // TODO: route non-manager reads to subsystem executors after they are implemented.
    return std::nullopt;
}

std::optional<database::ExecutorResult> DataManager::readVector(const contract::SubsystemKey& key)
{
    if (!db_ || state != DBState::LOADED) {
        return std::nullopt;
    }

    auto* executor = findExecutor(key.subsystem);
    if (!executor) {
        return database::ExecutorResult::Fail(database::ExecutorStatusCode::InternalError,
                                             "Executor is not registered");
    }

    return executor->ReadVector(*db_, key);
}

void DataManager::notifyObservers(const contract::UniterMessage& message)
{
    if (!message.resource) {
        qWarning() << "DataManager::notifyObservers(): message has no resource";
        return;
    }

    // A CUD payload contains the changed resource, so notification does not
    // need another database read. The same payload updates exact-resource and
    // list subscribers.
    const auto resourceKey = contract::ResourceKey::FromMessage(message);
    const auto listKey = contract::SubsystemKey::FromMessage(message);
    const uint64_t resourceId = message.resource->id;

    notifySingleResourceAdapters(resourceKey, message);
    notifyVectorResourceAdapters(listKey, message, resourceId);
}

void DataManager::notifySingleResourceAdapters(const contract::ResourceKey& key,
                                               const contract::UniterMessage& message)
{
    const auto range = resourceSubscribers_.equal_range(key);
    for (auto it = range.first; it != range.second;) {
        auto adapter = it->second;
        if (!adapter) {
            it = resourceSubscribers_.erase(it);
            continue;
        }

        adapter->updateData(message.crudact == contract::CrudAction::DELETE ? nullptr : message.resource,
                            message.crudact,
                            key.resourceId.value_or(0));
        ++it;
    }
}

void DataManager::notifyVectorResourceAdapters(const contract::SubsystemKey& key,
                                               const contract::UniterMessage& message,
                                               uint64_t resourceId)
{
    const auto range = resourceListSubscribers_.equal_range(key);
    for (auto it = range.first; it != range.second;) {
        auto adapter = it->second;
        if (!adapter) {
            it = resourceListSubscribers_.erase(it);
            continue;
        }

        adapter->updateData(message.crudact == contract::CrudAction::DELETE ? nullptr : message.resource,
                            message.crudact,
                            resourceId);
        ++it;
    }
}

} // namespace uniter::data
