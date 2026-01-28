#include "datamanager.h"
#include "../messages/unitermessage.h"

namespace uniter::data {

DataManager::DataManager(QObject* parent)
    : QObject(parent)
    , state(DBState::IDLE)
{
}

void DataManager::setState(DBState newState)
{
    state = newState;
}

void DataManager::onRecvUniterMessage(std::shared_ptr<messages::UniterMessage> message)
{
    // TODO: реализация
}

void DataManager::onStartLoadResources(QCryptographicHash& userhash)
{
    // TODO: реализация
}

void DataManager::onSubsystemGenerate(messages::Subsystem subsystem,
                                     messages::GenSubsystemType genType,
                                     uint64_t genId,
                                     bool created)
{
    // TODO: реализация
}

void DataManager::onCustomized()
{
    // TODO: реализация
}

void DataManager::onSubscribeToResourceList(messages::Subsystem subsystem,
                                           messages::ResourceType type)
{
    // TODO: реализация
}

void DataManager::onSubscribeToResourceTree(messages::Subsystem subsystem,
                                           messages::ResourceType type)
{
    // TODO: реализация
}

void DataManager::onSubscribeToResource(messages::Subsystem subsystem,
                                       messages::ResourceType type,
                                       uint64_t resId)
{
    // TODO: реализация
}

std::shared_ptr<resources::ResourceAbstract> DataManager::getResource(
    messages::Subsystem subsystem,
    messages::ResourceType type,
    uint64_t resourceId)
{
    // TODO: реализация
    return nullptr;
}


} // namespace uniter::data
