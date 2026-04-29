#ifndef UNITER_DATA_DATAADAPTER_H
#define UNITER_DATA_DATAADAPTER_H

#include "../contract/resourceabstract.h"
#include "../contract/unitermessage.h"
#include "../contract/uniterprotocol.h"
#include "../database/iresexecutor.h"
#include <QObject>
#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace uniter::data {

class SingleResourceAdapter;
class VectorResourceAdapter;

// Identifier for vector subscriptions.
struct AdapterKey {
    contract::Subsystem subsystem = contract::Subsystem::PROTOCOL;
    contract::GenSubsystem genSubsystem = contract::GenSubsystem::NOTGEN;
    std::optional<uint64_t> genId = std::nullopt;
    contract::ResourceType resourceType = contract::ResourceType::DEFAULT;

    bool operator<(const AdapterKey& other) const
    {
        return std::tie(subsystem, genSubsystem, genId, resourceType)
            < std::tie(other.subsystem, other.genSubsystem, other.genId, other.resourceType);
    }

    static std::optional<uint64_t> OptionalGenId(uint64_t genId)
    {
        return genId == 0 ? std::optional<uint64_t>{} : std::optional<uint64_t>{genId};
    }

    static AdapterKey FromMessage(const contract::UniterMessage& message)
    {
        // A message without resource still has routing context; DEFAULT keeps
        // the key valid for validation paths and READ requests.
        return AdapterKey{
            message.subsystem,
            message.GenSubsystem,
            OptionalGenId(message.GenSubsystemId),
            message.resource ? message.resource->resource_type : contract::ResourceType::DEFAULT
        };
    }

    static AdapterKey Context(contract::Subsystem subsystem,
                              contract::GenSubsystem genSubsystem,
                              const std::optional<uint64_t>& genId)
    {
        return AdapterKey{subsystem, genSubsystem, genId, contract::ResourceType::DEFAULT};
    }
};

// Identifier for single-resource subscriptions.
struct ResourceAdapterKey {
    AdapterKey adapterKey;
    uint64_t resourceId = 0;

    bool operator<(const ResourceAdapterKey& other) const
    {
        return std::tie(adapterKey, resourceId) < std::tie(other.adapterKey, other.resourceId);
    }

    static ResourceAdapterKey FromMessage(const contract::UniterMessage& message)
    {
        return ResourceAdapterKey{AdapterKey::FromMessage(message),
                                  message.resource ? message.resource->id : 0};
    }

    database::ResourceKey toDatabaseKey() const
    {
        // Resource id zero means "no exact id" in adapter API; executors use
        // std::nullopt for the same concept.
        return database::ResourceKey{
            adapterKey.subsystem,
            adapterKey.genSubsystem,
            adapterKey.genId,
            adapterKey.resourceType,
            resourceId == 0 ? std::optional<uint64_t>{} : std::optional<uint64_t>{resourceId}
        };
    }
};

// Single resource wrapper
class SingleResourceAdapter : public QObject
{
    Q_OBJECT

public:
    SingleResourceAdapter(ResourceAdapterKey key, QObject* parent = nullptr)
        : QObject(parent), key_(std::move(key)) {
        DataManager::instance()->subscribeResource(this);
    };
    SingleResourceAdapter(AdapterKey key, uint64_t resourceId, QObject* parent = nullptr)
        : SingleResourceAdapter(ResourceAdapterKey{std::move(key), resourceId}, parent) {};
    ~SingleResourceAdapter() override {
        DataManager::instance()->unsubscribeResource(this);
    };

    // Data container
    const ResourceAdapterKey& key() const { return key_; }
    uint64_t resourceId() const { return key_.resourceId; }
    std::shared_ptr<contract::ResourceAbstract> resource() const { return resource_; }

    // Initial fill is synchronous and deliberately silent: widgets react only
    // to later CUD notifications.
    void initializeData(std::shared_ptr<contract::ResourceAbstract> resource)
    {
        resource_ = std::move(resource);
    }

    // Callable from DataManager
    void updateData(std::shared_ptr<contract::ResourceAbstract> resource, contract::CrudAction action);

signals:
    void signalDataUpdated(SingleResourceAdapter* adapter, contract::CrudAction action);

private:
    ResourceAdapterKey key_;
    std::shared_ptr<contract::ResourceAbstract> resource_;
};

// Resource vector wrapper
class VectorResourceAdapter : public QObject
{
    Q_OBJECT

public:
    explicit VectorResourceAdapter(AdapterKey key, QObject* parent = nullptr) : QObject(parent)
    , key_(std::move(key)) {
        DataManager::instance()->subscribeResourceList(this);
    };
    VectorResourceAdapter(AdapterKey key, std::vector<std::shared_ptr<contract::ResourceAbstract>> resources, QObject* parent = nullptr)
    : QObject(parent), key_(std::move(key)), resources_(std::move(resources)) {
        DataManager::instance()->subscribeResourceList(this);
    };
    ~VectorResourceAdapter() override {
        DataManager::instance()->unsubscribeResourceList(this);
    };

    // Data container
    const AdapterKey& key() const { return key_; }
    const std::vector<std::shared_ptr<contract::ResourceAbstract>>& resources() const { return resources_; }

    // Same contract as SingleResourceAdapter::initializeData(): fill cache
    // without emitting signalDataUpdated.
    void initializeData(std::vector<std::shared_ptr<contract::ResourceAbstract>> resources)
    {
        resources_ = std::move(resources);
    }

    // Callable from DataManager
    void updateData(std::shared_ptr<contract::ResourceAbstract> resource,
                    contract::CrudAction action,
                    uint64_t resourceId);

signals:
    void signalDataUpdated(VectorResourceAdapter* adapter, uint64_t resourceId, contract::CrudAction action);

private:
    AdapterKey key_;
    std::vector<std::shared_ptr<contract::ResourceAbstract>> resources_;
};

} // namespace uniter::data

#endif // UNITER_DATA_DATAADAPTER_H
