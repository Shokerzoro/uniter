#ifndef IDATAOBSERVER_H
#define IDATAOBSERVER_H

#include "../contract/unitermessage.h"
#include "../contract/resourceabstract.h"
#include <QObject>
#include <memory>
#include <vector>
#include <optional>

/* Future note
To avoid synchronization issues, dangling observer pointers, and broad
broadcast fan-out, DataManager should use an adapter class.

The adapter is associated with an observer and is created when a subscription is
created. It connects to a specific observer, and all further communication with
that observer goes through the adapter.

This allows a proxy adapter to be used without connecting DataManager and
observers directly. Otherwise, notifying each observer separately would require
many slots.

This way data exchange uses Qt signals and slots. That removes the need for
synchronization and avoids dangling-pointer/direct-call problems, which are not
an issue for signal-slot communication.
*/


namespace uniter::data {

class SubscribeAdaptor {

};

// Subscription parameters structure
struct SubscriptionParams {
    contract::Subsystem subsystem;
    contract::ResourceType type;
    std::optional<uint64_t> resourceId; // Only for single resource
    bool isActive = false;

    SubscriptionParams() = default;
    SubscriptionParams(contract::Subsystem sub, contract::ResourceType t, std::optional<uint64_t> id = std::nullopt)
        : subsystem(sub), type(t), resourceId(id), isActive(false) {}
};


class IDataObserver : public QObject
{
    Q_OBJECT

protected:
    // Data containers
    std::shared_ptr<contract::ResourceAbstract> resourceData;
    std::vector<std::shared_ptr<contract::ResourceAbstract>> listData;
    std::shared_ptr<contract::ResourceAbstract> treeData; // Root node

    // Subscription options
    SubscriptionParams resourceParams;
    SubscriptionParams listParams;
    SubscriptionParams treeParams;

public:
    explicit IDataObserver(QObject* parent = nullptr);
    virtual ~IDataObserver() = default;

    // Methods for setting data (called by DataManager)
    void setResourceData(std::shared_ptr<contract::ResourceAbstract> resource);
    void setListData(std::vector<std::shared_ptr<contract::ResourceAbstract>> list);
    void setTreeData(std::shared_ptr<contract::ResourceAbstract> rootNode);

    // Data retrieval methods (for widgets)
    std::shared_ptr<contract::ResourceAbstract> getResourceData() const;
    const std::vector<std::shared_ptr<contract::ResourceAbstract>>& getListData() const;
    std::shared_ptr<contract::ResourceAbstract> getTreeData() const;

    // Accessing subscription options
    const SubscriptionParams& getResourceParams() const { return resourceParams; }
    const SubscriptionParams& getListParams() const { return listParams; }
    const SubscriptionParams& getTreeParams() const { return treeParams; }

signals:
    // Signals for subscription (connect to DataManager in constructor)
    void subscribeToResource(contract::Subsystem subsystem,
                             contract::ResourceType type,
                             std::optional<uint64_t> resId,
                             QObject* observer);

    void subscribeToResourceList(contract::Subsystem subsystem,
                                 contract::ResourceType type,
                                 QObject* observer);

    void subscribeToResourceTree(contract::Subsystem subsystem,
                                 contract::ResourceType type,
                                 QObject* observer);

    // Signals for unsubscribing
    void unsubscribeFromResource(QObject* observer);
    void unsubscribeFromResourceList(QObject* observer);
    void unsubscribeFromResourceTree(QObject* observer);

    // Notification of new data receipt
    void dataUpdated();

protected:
    // Helper Methods for Heirs
    void requestResourceSubscription(contract::Subsystem subsystem,
                                     contract::ResourceType type,
                                     uint64_t resId);

    void requestListSubscription(contract::Subsystem subsystem,
                                 contract::ResourceType type);

    void requestTreeSubscription(contract::Subsystem subsystem,
                                 contract::ResourceType type);

    void cancelResourceSubscription();
    void cancelListSubscription();
    void cancelTreeSubscription();
};

} // namespace uniter::data

#endif // IDATAOBSERVER_H
