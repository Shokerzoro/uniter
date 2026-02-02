#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "../contract/unitermessage.h"
#include "../contract/resourceabstract.h"
#include "idataobserver.h"
#include <QObject>
#include <QByteArray>
#include <memory>
#include <vector>


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

    // Списки обзерверов
    std::vector<std::weak_ptr<IDataObserver>> treeObservers;
    std::vector<std::weak_ptr<IDataObserver>> listObservers;
    std::vector<std::weak_ptr<IDataObserver>> resourceObservers;

    // Очистка устаревших weak_ptr
    void cleanupObservers();

public:
    // Публичный статический метод получения экземпляра
    static DataManager* instance();

    ~DataManager() override = default;

public slots:
    void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message);
    void onStartLoadResources(QByteArray userhash); // От менеджера приложения
    void onSubsystemGenerate(contract::Subsystem subsystem,
                             contract::GenSubsystemType genType,
                             uint64_t genId,
                             bool created);

    // Подписки на ресурсы
    void onSubscribeToResourceList(contract::Subsystem subsystem,
                                   contract::ResourceType type,
                                   std::shared_ptr<IDataObserver> observer);
    void onSubscribeToResourceTree(contract::Subsystem subsystem,
                                   contract::ResourceType type,
                                   std::shared_ptr<IDataObserver> observer);
    void onSubscribeToResource(contract::Subsystem subsystem,
                               contract::ResourceType type,
                               uint64_t resId,
                               std::shared_ptr<IDataObserver> observer);

    // Отписки от ресурсов
    void onUnsubscribeFromResourceList(std::shared_ptr<IDataObserver> observer);
    void onUnsubscribeFromResourceTree(std::shared_ptr<IDataObserver> observer);
    void onUnsubscribeFromResource(std::shared_ptr<IDataObserver> observer);

    // Получить ресурс
    void onGetResource(contract::Subsystem subsystem,
                       contract::ResourceType type,
                       uint64_t resourceId,
                       std::shared_ptr<IDataObserver> observer);

signals:
    void signalResourcesLoaded(); // Менеджеру приложения
};

} // namespace uniter::data

#endif // DATAMANAGER_H
