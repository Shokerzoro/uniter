#ifndef UNITER_DATA_DATAADAPTER_H
#define UNITER_DATA_DATAADAPTER_H

#include "../contract/resourceabstract.h"
#include "../contract/uniterprotocol.h"
#include <QObject>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace uniter::data {

class DataAdapter : public QObject
{
public:
    explicit DataAdapter(QObject* parent = nullptr) : QObject(parent) {}
    ~DataAdapter() override = default;

    virtual const contract::SubsystemKey& subsystemKey() const = 0;
    virtual std::optional<contract::ResourceKey> resourceKey() const = 0;
    virtual void initializeData(std::shared_ptr<contract::ResourceAbstract> resource) = 0;
    virtual void initializeData(std::vector<std::shared_ptr<contract::ResourceAbstract>> resources)
    {
        (void)resources;
    }
    virtual void updateData(std::shared_ptr<contract::ResourceAbstract> resource,
                            contract::CrudAction action,
                            uint64_t resourceId) = 0;
};

class SingleResourceAdapter;
class VectorResourceAdapter;

// Single resource wrapper
class SingleResourceAdapter : public DataAdapter
{
    Q_OBJECT

public:
    SingleResourceAdapter(contract::ResourceKey key, QObject* parent = nullptr);
    SingleResourceAdapter(contract::SubsystemKey key, uint64_t resourceId, QObject* parent = nullptr);
    ~SingleResourceAdapter() override;

    // Data container
    const contract::ResourceKey& key() const { return key_; }
    const contract::SubsystemKey& subsystemKey() const override { return key_.subsystemKey; }
    std::optional<contract::ResourceKey> resourceKey() const override { return key_; }
    uint64_t resourceId() const { return key_.resourceId.value_or(0); }
    std::shared_ptr<contract::ResourceAbstract> resource() const { return resource_; }

    // Initial fill is synchronous and deliberately silent: widgets react only
    // to later CUD notifications.
    void initializeData(std::shared_ptr<contract::ResourceAbstract> resource) override
    {
        resource_ = std::move(resource);
    }

    // Callable from DataManager
    void updateData(std::shared_ptr<contract::ResourceAbstract> resource, contract::CrudAction action);
    void updateData(std::shared_ptr<contract::ResourceAbstract> resource,
                    contract::CrudAction action,
                    uint64_t resourceId) override;

signals:
    void signalDataUpdated(SingleResourceAdapter* adapter, contract::CrudAction action);

private:
    contract::ResourceKey key_;
    std::shared_ptr<contract::ResourceAbstract> resource_;
};

// Resource vector wrapper
class VectorResourceAdapter : public DataAdapter
{
    Q_OBJECT

public:
    explicit VectorResourceAdapter(contract::SubsystemKey key, QObject* parent = nullptr);
    VectorResourceAdapter(contract::SubsystemKey key,
                          std::vector<std::shared_ptr<contract::ResourceAbstract>> resources,
                          QObject* parent = nullptr);
    ~VectorResourceAdapter() override;

    // Data container
    const contract::SubsystemKey& key() const { return key_; }
    const contract::SubsystemKey& subsystemKey() const override { return key_; }
    std::optional<contract::ResourceKey> resourceKey() const override { return std::nullopt; }
    const std::vector<std::shared_ptr<contract::ResourceAbstract>>& resources() const { return resources_; }

    void initializeData(std::shared_ptr<contract::ResourceAbstract> resource) override
    {
        (void)resource;
    }

    // Same contract as SingleResourceAdapter::initializeData(): fill cache
    // without emitting signalDataUpdated.
    void initializeData(std::vector<std::shared_ptr<contract::ResourceAbstract>> resources) override
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
    contract::SubsystemKey key_;
    std::vector<std::shared_ptr<contract::ResourceAbstract>> resources_;
};

} // namespace uniter::data

#endif // UNITER_DATA_DATAADAPTER_H
