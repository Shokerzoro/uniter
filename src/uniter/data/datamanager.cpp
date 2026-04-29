#include "datamanager.h"
#include "sqlitedatabase.h"
#include "../../common/appfuncs.h"
#include "../database/transaction.h"
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
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

    if (event == DBEvent::FAIL) {
        setState(DBState::ERROR);
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
        if (event == DBEvent::INIT_DATABASE) {
            setState(DBState::LOADING);
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

    // List reads are not stable yet, so vector adapters may be constructed with
    // initial data by their owner. DataManager only registers future updates.
    resourceListSubscribers_.insert({adapter->subsystemKey(), QPointer<DataAdapter>(adapter)});

    connect(adapter, &QObject::destroyed, this, [this, adapter]() {
        unsubscribeResourceList(adapter);
    });
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
    (void)userhash;
    processEvent(DBEvent::INIT_DATABASE);
    processEvent(DBEvent::RESOURCES_LOADED);
    emit signalResourcesLoaded();
}

void DataManager::onClearResources()
{
    processEvent(DBEvent::CLEAR_RESOURCES);
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
        switch (message.crudact) {
            case contract::CrudAction::CREATE:
                // DataManager routes messages; concrete DB work stays inside the subsystem executor.
                return managerExecutor_.Create(*db_, message);
            case contract::CrudAction::READ:
                // READ uses the executor API for explicit resource requests only.
                return managerExecutor_.Read(*db_, contract::ResourceKey::FromMessage(message));
            case contract::CrudAction::UPDATE:
                // DataManager does not inspect or mutate tables directly.
                return managerExecutor_.Update(*db_, message);
            case contract::CrudAction::DELETE:
                // After successful executor delete, observer adapters are notified below.
                return managerExecutor_.Delete(*db_, message);
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
        return managerExecutor_.Read(*db_, key);
    }

    // TODO: route non-manager reads to subsystem executors after they are implemented.
    return std::nullopt;
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
