#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "../contract/unitermessage.h"
#include "../database/common/commonexecutor.h"
#include "../database/idatabase.h"
#include "../database/manager/managerexecutor.h"
#include <QObject>
#include <QByteArray>
#include <memory>
#include <optional>
#include <string>


namespace uniter::data {

class DataManager : public QObject
{
    Q_OBJECT

private:
    // Private constructor for singleton
    DataManager();

    // Prohibition of copying and moving
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;
    DataManager(DataManager&&) = delete;
    DataManager& operator=(DataManager&&) = delete;

    // State Management
    enum class DBState { IDLE, LOADING, LOADED, ERROR };
    DBState state = DBState::IDLE;
    void setState(DBState newState);

    std::unique_ptr<database::IDataBase> db_;
    database::CommonExecutor commonExecutor_;
    database::ManagerExecutor managerExecutor_;
    QByteArray userHash_;
    QString databasePath_;

    QString resolveDatabasePath(const QByteArray& userhash) const;
    bool initializeDatabase(const QByteArray& userhash);
    bool initializeExecutor(database::IResExecutor& executor);
    bool clearExecutorData(database::IResExecutor& executor);
    std::optional<database::ExecutorResult> routeManagerMessage(const contract::UniterMessage& message);

public:
    // Public static method to get an instance
    static DataManager* instance();

    ~DataManager() override = default;

public slots:
    // From the application manager
    void onStartLoadResources(QByteArray userhash);
    void onClearResources();
    // Complete database cleanup (DBCLEAR): drop/empty all resource tables,
    // leave the structure. Upon completion, the signal signalDatabaseCleared.
    void onClearDatabase();
    void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message);
    void onSubsystemGenerate(contract::Subsystem subsystem,
                             contract::GenSubsystem genType,
                             std::optional<uint64_t> genId,
                             bool created);

signals:
    void signalResourcesLoaded();  // Application manager: DB loaded
    void signalDatabaseCleared();  // To the application manager: the database is completely cleared (DBCLEAR)
};

} // namespace uniter::data

#endif // DATAMANAGER_H
