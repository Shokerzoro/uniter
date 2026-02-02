#ifndef IDATAOBSERVER_H
#define IDATAOBSERVER_H

#include "../contract/unitermessage.h"
#include "../contract/resourceabstract.h"
#include <QObject>
#include <memory>
#include <vector>
#include <optional>


namespace uniter::data {

// Структура параметров подписки
struct SubscriptionParams {
    contract::Subsystem subsystem;
    contract::ResourceType type;
    std::optional<uint64_t> resourceId; // Только для single resource
    bool isActive = false;

    SubscriptionParams() = default;
    SubscriptionParams(contract::Subsystem sub, contract::ResourceType t, std::optional<uint64_t> id = std::nullopt)
        : subsystem(sub), type(t), resourceId(id), isActive(false) {}
};


class IDataObserver : public QObject
{
    Q_OBJECT

protected:
    // Контейнеры для данных
    std::shared_ptr<contract::ResourceAbstract> resourceData;
    std::vector<std::shared_ptr<contract::ResourceAbstract>> listData;
    std::shared_ptr<contract::ResourceAbstract> treeData; // Корневой узел

    // Параметры подписок
    SubscriptionParams resourceParams;
    SubscriptionParams listParams;
    SubscriptionParams treeParams;

public:
    explicit IDataObserver(QObject* parent = nullptr);
    virtual ~IDataObserver() = default;

    // Методы для установки данных (вызываются DataManager'ом)
    void setResourceData(std::shared_ptr<contract::ResourceAbstract> resource);
    void setListData(std::vector<std::shared_ptr<contract::ResourceAbstract>> list);
    void setTreeData(std::shared_ptr<contract::ResourceAbstract> rootNode);

    // Методы получения данных (для виджетов)
    std::shared_ptr<contract::ResourceAbstract> getResourceData() const;
    const std::vector<std::shared_ptr<contract::ResourceAbstract>>& getListData() const;
    std::shared_ptr<contract::ResourceAbstract> getTreeData() const;

    // Доступ к параметрам подписок
    const SubscriptionParams& getResourceParams() const { return resourceParams; }
    const SubscriptionParams& getListParams() const { return listParams; }
    const SubscriptionParams& getTreeParams() const { return treeParams; }

signals:
    // Сигналы для подписки (соединяются с DataManager в конструкторе)
    void subscribeToResource(contract::Subsystem subsystem,
                             contract::ResourceType type,
                             uint64_t resId,
                             std::shared_ptr<IDataObserver> observer);

    void subscribeToResourceList(contract::Subsystem subsystem,
                                 contract::ResourceType type,
                                 std::shared_ptr<IDataObserver> observer);

    void subscribeToResourceTree(contract::Subsystem subsystem,
                                 contract::ResourceType type,
                                 std::shared_ptr<IDataObserver> observer);

    // Сигналы для отписки
    void unsubscribeFromResource(std::shared_ptr<IDataObserver> observer);
    void unsubscribeFromResourceList(std::shared_ptr<IDataObserver> observer);
    void unsubscribeFromResourceTree(std::shared_ptr<IDataObserver> observer);

    // Уведомление о получении новых данных
    void dataUpdated();

protected:
    // Вспомогательные методы для наследников
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
