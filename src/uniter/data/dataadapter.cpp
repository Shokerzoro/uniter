#include "dataadapter.h"
#include "datamanager.h"
#include <algorithm>
#include <utility>

namespace uniter::data {

SingleResourceAdapter::SingleResourceAdapter(contract::ResourceKey key, QObject* parent)
    : DataAdapter(parent)
    , key_(std::move(key))
{
    DataManager::instance()->subscribeResource(this);
}

SingleResourceAdapter::SingleResourceAdapter(contract::SubsystemKey key, uint64_t resourceId, QObject* parent)
    : SingleResourceAdapter(contract::ResourceKey{std::move(key), std::optional<uint64_t>{resourceId}}, parent)
{
}

SingleResourceAdapter::~SingleResourceAdapter()
{
    DataManager::instance()->unsubscribeResource(this);
}

void SingleResourceAdapter::updateData(std::shared_ptr<contract::ResourceAbstract> resource,
                                       contract::CrudAction action)
{
    resource_ = std::move(resource);
    emit signalDataUpdated(this, action);
}

void SingleResourceAdapter::updateData(std::shared_ptr<contract::ResourceAbstract> resource,
                                       contract::CrudAction action,
                                       uint64_t resourceId)
{
    (void)resourceId;
    updateData(std::move(resource), action);
}

VectorResourceAdapter::VectorResourceAdapter(contract::SubsystemKey key, QObject* parent)
    : DataAdapter(parent)
    , key_(std::move(key))
{
    DataManager::instance()->subscribeResourceList(this);
}

VectorResourceAdapter::VectorResourceAdapter(contract::SubsystemKey key,
                                             std::vector<std::shared_ptr<contract::ResourceAbstract>> resources,
                                             QObject* parent)
    : DataAdapter(parent)
    , key_(std::move(key))
    , resources_(std::move(resources))
{
    DataManager::instance()->subscribeResourceList(this);
}

VectorResourceAdapter::~VectorResourceAdapter()
{
    DataManager::instance()->unsubscribeResourceList(this);
}

void VectorResourceAdapter::updateData(std::shared_ptr<contract::ResourceAbstract> resource,
                                       contract::CrudAction action,
                                       uint64_t resourceId)
{
    auto it = std::find_if(resources_.begin(), resources_.end(),
                           [resourceId](const std::shared_ptr<contract::ResourceAbstract>& item) {
                               return item && item->id == resourceId;
                           });

    if (action == contract::CrudAction::DELETE) {
        if (it != resources_.end()) {
            resources_.erase(it);
        }
        emit signalDataUpdated(this, resourceId, action);
        return;
    }

    if (!resource) {
        return;
    }

    if (it == resources_.end()) {
        resources_.push_back(std::move(resource));
    } else {
        *it = std::move(resource);
    }

    emit signalDataUpdated(this, resourceId, action);
}

} // namespace uniter::data
