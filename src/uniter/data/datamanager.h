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
    // Приватный конструктор для синглтона
    DataManager();

    // Запрет копирования и перемещения
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
    // Публичный статический метод получения экземпляра
    static DataManager* instance();

    ~DataManager() override = default;

public slots:
    // От менеджера приложения
    void onStartLoadResources(QByteArray userhash);
    void onClearResources();
    // Полная очистка БД (DBCLEAR): дропнуть/опустошить все таблицы ресурсов,
    // оставить структуру. По завершении — сигнал signalDatabaseCleared.
    void onClearDatabase();
    void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message);
    void onSubsystemGenerate(contract::Subsystem subsystem,
                             contract::GenSubsystem genType,
                             std::optional<uint64_t> genId,
                             bool created);

signals:
    void signalResourcesLoaded();  // Менеджеру приложения: БД загружена
    void signalDatabaseCleared();  // Менеджеру приложения: БД полностью очищена (DBCLEAR)
};

} // namespace uniter::data

#endif // DATAMANAGER_H
