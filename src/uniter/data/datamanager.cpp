
#include "datamanager.h"
#include "../contract/unitermessage.h"
#include <QByteArray>
#include <algorithm>

namespace uniter::data {

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

void DataManager::cleanupObservers()
{
    auto cleanup = [](auto& observers) {
        observers.erase(
            std::remove_if(observers.begin(), observers.end(),
                           [](const auto& wp) { return wp.expired(); }),
            observers.end()
            );
    };

    cleanup(treeObservers);
    cleanup(listObservers);
    cleanup(resourceObservers);
}

void DataManager::onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message)
{
    // TODO: реализация обработки входящих CRUD операций
    // Обновление локальной БД/кэша
    // Уведомление обзерверов через вызов их методов напрямую
}

void DataManager::onStartLoadResources(QByteArray userhash)
{
    // TODO: реализация загрузки ресурсов из БД
    emit signalResourcesLoaded();
}

void DataManager::onSubsystemGenerate(contract::Subsystem subsystem,
                                      contract::GenSubsystemType genType,
                                      uint64_t genId,
                                      bool created)
{
    // TODO: реализация
}

void DataManager::onSubscribeToResourceList(contract::Subsystem subsystem,
                                            contract::ResourceType type,
                                            std::shared_ptr<IDataObserver> observer)
{
    if (observer) {
        listObservers.push_back(observer);
        cleanupObservers();
    }
    // TODO: загрузить данные и передать в observer->setListData()
}

void DataManager::onSubscribeToResourceTree(contract::Subsystem subsystem,
                                            contract::ResourceType type,
                                            std::shared_ptr<IDataObserver> observer)
{
    if (observer) {
        treeObservers.push_back(observer);
        cleanupObservers();
    }
    // TODO: загрузить данные и передать в observer->setTreeData()
}

void DataManager::onSubscribeToResource(contract::Subsystem subsystem,
                                        contract::ResourceType type,
                                        uint64_t resId,
                                        std::shared_ptr<IDataObserver> observer)
{
    if (observer) {
        resourceObservers.push_back(observer);
        cleanupObservers();
    }
    // TODO: загрузить данные и передать в observer->setResourceData()
}

void DataManager::onUnsubscribeFromResourceList(std::shared_ptr<IDataObserver> observer)
{
    if (!observer) return;

    listObservers.erase(
        std::remove_if(listObservers.begin(), listObservers.end(),
                       [&observer](const std::weak_ptr<IDataObserver>& wp) {
                           auto sp = wp.lock();
                           return !sp || sp == observer;
                       }),
        listObservers.end()
        );
}

void DataManager::onUnsubscribeFromResourceTree(std::shared_ptr<IDataObserver> observer)
{
    if (!observer) return;

    treeObservers.erase(
        std::remove_if(treeObservers.begin(), treeObservers.end(),
                       [&observer](const std::weak_ptr<IDataObserver>& wp) {
                           auto sp = wp.lock();
                           return !sp || sp == observer;
                       }),
        treeObservers.end()
        );
}

void DataManager::onUnsubscribeFromResource(std::shared_ptr<IDataObserver> observer)
{
    if (!observer) return;

    resourceObservers.erase(
        std::remove_if(resourceObservers.begin(), resourceObservers.end(),
                       [&observer](const std::weak_ptr<IDataObserver>& wp) {
                           auto sp = wp.lock();
                           return !sp || sp == observer;
                       }),
        resourceObservers.end()
        );
}

void DataManager::onGetResource(
    contract::Subsystem subsystem,
    contract::ResourceType type,
    uint64_t resourceId,
    std::shared_ptr<IDataObserver> observer)
{
    // TODO: реализация
    // Вернуть ресурс через вызов метода observer->setResourceData()
}

} // namespace uniter::data

