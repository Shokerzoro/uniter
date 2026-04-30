#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "dataadapter.h"
#include "../contract/unitermessage.h"
#include "../database/common/commonexecutor.h"
#include "../database/idatabase.h"
#include "../database/iresexecutor.h"
#include "../database/manager/managerexecutor.h"
#include <QObject>
#include <QByteArray>
#include <QPointer>
#include <map>
#include <memory>
#include <optional>

namespace uniter::data {

class DataManager : public QObject
{
    Q_OBJECT

private:
    DataManager();

    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;
    DataManager(DataManager&&) = delete;
    DataManager& operator=(DataManager&&) = delete;

    // State/events separation for right FSM implementation
    enum class DBState { IDLE, LOADING, LOADED, CLEARING, ERROR };
    enum class DBEvent { INIT_DATABASE, RESOURCES_LOADED, CLEAR_RESOURCES, RESOURCES_CLEARED, RESET_DATABASE, FAIL };

    DBState state = DBState::IDLE;
    void setState(DBState newState);
    bool processEvent(DBEvent event);

    std::unique_ptr<database::IDataBase> db_;
    database::CommonExecutor commonExecutor_;
    database::ManagerExecutor managerExecutor_;

    QByteArray userHash_;
    QString databasePath_;
    QString resolveDatabasePath(const QByteArray& userhash) const;
    bool openAndInitializeDatabase();
    bool dropAndReinitializeDatabase();
    void failLoading();

    std::multimap<contract::ResourceKey, QPointer<DataAdapter>> resourceSubscribers_;
    std::multimap<contract::SubsystemKey, QPointer<DataAdapter>> resourceListSubscribers_;
    std::map<contract::SubsystemKey, bool> activeSubsystemContexts_;

    void notifyObservers(const contract::UniterMessage& message);
    void notifySingleResourceAdapters(const contract::ResourceKey& key,
                                      const contract::UniterMessage& message);
    void notifyVectorResourceAdapters(const contract::SubsystemKey& key,
                                      const contract::UniterMessage& message,
                                      uint64_t resourceId);

    std::optional<database::ExecutorResult> routeMessage(const contract::UniterMessage& message);
    std::optional<database::ExecutorResult> readResource(const contract::ResourceKey& key);

public:
    static DataManager* instance();
    ~DataManager() override = default;

    void subscribeResource(DataAdapter* adapter);
    void subscribeResourceList(DataAdapter* adapter);
    void unsubscribeResource(DataAdapter* adapter);
    void unsubscribeResourceList(DataAdapter* adapter);

#ifdef UNITER_TESTS
    void notifyObserversForTest(const contract::UniterMessage& message) { notifyObservers(message); }
#endif

public slots:
    // CUD message from AppManager
    void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message);

    // From AppManager
    void onInitDatabase(QByteArray userhash);
    void onClearResources();
    void onResetDatabase();

    void onSubsystemGenerate(contract::Subsystem subsystem,
                             contract::GenSubsystem genType,
                             std::optional<uint64_t> genId,
                             bool created);

signals:
    // Signals to AppManager
    void signalResourcesLoaded(bool loaded);
    void signalResourcesCleared();
};

} // namespace uniter::data

#endif // DATAMANAGER_H
