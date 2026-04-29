#include "dataadapter.h"
#include "datamanager.h"
#include <algorithm>
#include <utility>

namespace uniter::data {


void SingleResourceAdapter::updateData(std::shared_ptr<contract::ResourceAbstract> resource,
                                       contract::CrudAction action)
{
    resource_ = std::move(resource);
    emit signalDataUpdated(this, action);
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
