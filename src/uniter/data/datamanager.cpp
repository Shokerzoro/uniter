
#include "datamanager.h"
#include "../contract/unitermessage.h"
#include <QByteArray>
#include <optional>
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

}

void DataManager::onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message)
{

}

void DataManager::onStartLoadResources(QByteArray userhash)
{
    // TODO: реализация загрузки ресурсов из БД
    emit signalResourcesLoaded();
}

void DataManager::onSubsystemGenerate(contract::Subsystem subsystem,
                                      contract::GenSubsystemType genType,
                                      std::optional<uint64_t> genId,
                                      bool created)
{
    // TODO: реализация
}

void DataManager::onSubscribeToResourceList(contract::Subsystem subsystem,
                                            contract::ResourceType type,
                                            QObject* observer)
{

}

void DataManager::onSubscribeToResourceTree(contract::Subsystem subsystem,
                                            contract::ResourceType type,
                                            QObject* observer)
{

}

void DataManager::onSubscribeToResource(contract::Subsystem subsystem,
                                        contract::ResourceType type,
                                        std::optional<uint64_t> resId,
                                        QObject* observer)
{

}

void DataManager::onUnsubscribeFromResourceList(QObject* observer)
{

}

void DataManager::onUnsubscribeFromResourceTree(QObject* observer)
{

}

void DataManager::onUnsubscribeFromResource(QObject* observer)
{

}

void DataManager::onGetResource(
    contract::Subsystem subsystem,
    contract::ResourceType type,
    std::optional<uint64_t> resourceId,
    QObject* observer)
{

}

} // namespace uniter::data

