#include "idataobserver.h"
#include "datamanager.h"

namespace uniter::data {

IDataObserver::IDataObserver(QObject* parent)
    : QObject(parent)
    , resourceData(nullptr)
    , treeData(nullptr)
{
    // Подключение сигналов к слотам DataManager
    auto* dm = DataManager::instance();

    connect(this, &IDataObserver::subscribeToResource,
            dm, &DataManager::onSubscribeToResource);

    connect(this, &IDataObserver::subscribeToResourceList,
            dm, &DataManager::onSubscribeToResourceList);

    connect(this, &IDataObserver::subscribeToResourceTree,
            dm, &DataManager::onSubscribeToResourceTree);

    connect(this, &IDataObserver::unsubscribeFromResource,
            dm, &DataManager::onUnsubscribeFromResource);

    connect(this, &IDataObserver::unsubscribeFromResourceList,
            dm, &DataManager::onUnsubscribeFromResourceList);

    connect(this, &IDataObserver::unsubscribeFromResourceTree,
            dm, &DataManager::onUnsubscribeFromResourceTree);
}

void IDataObserver::setResourceData(std::shared_ptr<contract::ResourceAbstract> resource)
{
    resourceData = resource;
    emit dataUpdated();
}

void IDataObserver::setListData(std::vector<std::shared_ptr<contract::ResourceAbstract>> list)
{
    listData = std::move(list);
    emit dataUpdated();
}

void IDataObserver::setTreeData(std::shared_ptr<contract::ResourceAbstract> rootNode)
{
    treeData = rootNode;
    emit dataUpdated();
}

std::shared_ptr<contract::ResourceAbstract> IDataObserver::getResourceData() const
{
    return resourceData;
}

const std::vector<std::shared_ptr<contract::ResourceAbstract>>& IDataObserver::getListData() const
{
    return listData;
}

std::shared_ptr<contract::ResourceAbstract> IDataObserver::getTreeData() const
{
    return treeData;
}

void IDataObserver::requestResourceSubscription(contract::Subsystem subsystem,
                                                contract::ResourceType type,
                                                uint64_t resId)
{
    resourceParams = SubscriptionParams(subsystem, type, resId);
    resourceParams.isActive = true;

    emit subscribeToResource(subsystem, type, resId, this);
}

void IDataObserver::requestListSubscription(contract::Subsystem subsystem,
                                           contract::ResourceType type)
{
    listParams = SubscriptionParams(subsystem, type);
    listParams.isActive = true;

    emit subscribeToResourceList(subsystem, type, this);
}

void IDataObserver::requestTreeSubscription(contract::Subsystem subsystem,
                                           contract::ResourceType type)
{
    treeParams = SubscriptionParams(subsystem, type);
    treeParams.isActive = true;

    emit subscribeToResourceTree(subsystem, type, this);
}

void IDataObserver::cancelResourceSubscription()
{
    if (!resourceParams.isActive) return;

    resourceParams.isActive = false;
    emit unsubscribeFromResource(this);
}

void IDataObserver::cancelListSubscription()
{
    if (!listParams.isActive) return;

    listParams.isActive = false;
    emit unsubscribeFromResourceList(this);
}

void IDataObserver::cancelTreeSubscription()
{
    if (!treeParams.isActive) return;

    treeParams.isActive = false;
    emit unsubscribeFromResourceTree(this);
}

} // namespace uniter::data
