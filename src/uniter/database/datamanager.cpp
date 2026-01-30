#include "datamanager.h"
#include "../messages/unitermessage.h"
#include <QByteArray>

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

void DataManager::onStartLoadResources(QByteArray userhash)
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
                                            messages::ResourceType type,
                                            std::shared_ptr<genwdg::ISubsWdg> observer)
{
    // TODO: реализация
}

void DataManager::onSubscribeToResourceTree(messages::Subsystem subsystem,
                                            messages::ResourceType type,
                                            std::shared_ptr<genwdg::ISubsWdg> observer)
{
    // TODO: реализация
}

void DataManager::onSubscribeToResource(messages::Subsystem subsystem,
                                        messages::ResourceType type,
                                        uint64_t resId,
                                        std::shared_ptr<genwdg::ISubsWdg> observer)
{
    // TODO: реализация
}

void DataManager::onGetResource(
    messages::Subsystem subsystem,
    messages::ResourceType type,
    uint64_t resourceId,
    std::shared_ptr<genwdg::ISubsWdg> observer)
{
    // TODO: реализация
}


} // namespace uniter::data
