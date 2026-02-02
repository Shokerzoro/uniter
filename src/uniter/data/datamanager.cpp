#include "datamanager.h"
#include "../messages/unitermessage.h"
#include <QByteArray>

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

void DataManager::onRecvUniterMessage(std::shared_ptr<messages::UniterMessage> message)
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
    // Добавить observer в listObservers
}

void DataManager::onSubscribeToResourceTree(messages::Subsystem subsystem,
                                            messages::ResourceType type,
                                            std::shared_ptr<genwdg::ISubsWdg> observer)
{
    // TODO: реализация
    // Добавить observer в treeObservers
}

void DataManager::onSubscribeToResource(messages::Subsystem subsystem,
                                        messages::ResourceType type,
                                        uint64_t resId,
                                        std::shared_ptr<genwdg::ISubsWdg> observer)
{
    // TODO: реализация
    // Добавить observer в resourceObservers
}

void DataManager::onGetResource(
    messages::Subsystem subsystem,
    messages::ResourceType type,
    uint64_t resourceId,
    std::shared_ptr<genwdg::ISubsWdg> observer)
{
    // TODO: реализация
    // Вернуть ресурс через вызов метода observer
}


} // namespace uniter::data
