// idataobserver.cpp
#include "idataobserver.h"
#include "datamanager.h"

namespace uniter::data {

IDataObserver::IDataObserver(DataManager* dataManager, QObject *parent)
    : QObject(parent)
    , m_dataManager(dataManager)
{
    // ✅ Коннектим сигналы к слотам DataManager
    connect(this, &IDataObserver::signalSubscribeToList,
            m_dataManager, &DataManager::onSubscribeToResourceList);

    connect(this, &IDataObserver::signalSubscribeToTree,
            m_dataManager, &DataManager::onSubscribeToResourceTree);

    connect(this, &IDataObserver::signalSubscribeToResource,
            m_dataManager, &DataManager::onSubscribeToResource);
}

IDataObserver::~IDataObserver()
{
}

// ✅ Виджет вызывает эти методы для подписки
void IDataObserver::subscribeToList(contract::Subsystem subsystem, contract::ResourceType type)
{
    m_listSubscription.subsystem = subsystem;
    m_listSubscription.type = type;

    // ✅ Испускаем сигнал - он уже законнекчен к DataManager
    emit signalSubscribeToList(subsystem, type, this);
}

void IDataObserver::subscribeToTree(contract::Subsystem subsystem, contract::ResourceType type)
{
    m_treeSubscription.subsystem = subsystem;
    m_treeSubscription.type = type;

    emit signalSubscribeToTree(subsystem, type, this);
}

void IDataObserver::subscribeToResource(contract::Subsystem subsystem, contract::ResourceType type, uint64_t resId)
{
    m_resourceSubscription.subsystem = subsystem;
    m_resourceSubscription.type = type;
    m_resourceSubscription.resourceId = resId;

    emit signalSubscribeToResource(subsystem, type, resId, this);
}

// DataManager вызывает эти методы напрямую
void IDataObserver::notifyListObservers(contract::Subsystem subsystem,
                                       contract::ResourceType type,
                                       std::vector<std::shared_ptr<contract::ResourceAbstract>> data)
{
    if (m_listSubscription.subsystem == subsystem && m_listSubscription.type == type) {
        m_listData = std::move(data);
        emit listDataChanged();
    }
}

void IDataObserver::notifyTreeObservers(contract::Subsystem subsystem,
                                       contract::ResourceType type,
                                       std::vector<std::shared_ptr<contract::ResourceAbstract>> data)
{
    if (m_treeSubscription.subsystem == subsystem && m_treeSubscription.type == type) {
        m_treeData = std::move(data);
        emit treeDataChanged();
    }
}

void IDataObserver::notifyResourceObservers(contract::Subsystem subsystem,
                                           contract::ResourceType type,
                                           uint64_t resId,
                                           std::shared_ptr<contract::ResourceAbstract> resource)
{
    if (m_resourceSubscription.subsystem == subsystem &&
        m_resourceSubscription.type == type &&
        m_resourceSubscription.resourceId == resId) {
        m_resourceData = resource;
        emit resourceDataChanged();
    }
}

} // namespace uniter::data
