// datamanager.h
#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "../contract/unitermessage.h"
#include "../contract/resourceabstract.h"
#include "idataobserver.h"
#include <QObject>
#include <QPointer>
#include <QByteArray>
#include <memory>
#include <map>
#include <vector>

namespace uniter::data {

class DataManager : public QObject
{
    Q_OBJECT
public:
    explicit DataManager(QObject* parent = nullptr);  // ✅ Публичный конструктор!
    ~DataManager() override = default;

private:
    // State Management
    enum class DBState { IDLE, LOADING, LOADED, ERROR };
    DBState state = DBState::IDLE;
    void setState(DBState newState);

    // Структура для хранения observers
    struct ObserverKey {
        contract::Subsystem subsystem;
        contract::ResourceType type;
        uint64_t resourceId = 0;

        bool operator<(const ObserverKey& other) const {
            if (subsystem != other.subsystem) return subsystem < other.subsystem;
            if (type != other.type) return type < other.type;
            return resourceId < other.resourceId;
        }
    };

    // Три списка observers
    std::map<ObserverKey, std::vector<QPointer<IDataObserver>>> m_listObservers;
    std::map<ObserverKey, std::vector<QPointer<IDataObserver>>> m_treeObservers;
    std::map<ObserverKey, std::vector<QPointer<IDataObserver>>> m_resourceObservers;

    // Приватные методы для уведомления
    void notifyListObservers(contract::Subsystem subsystem, contract::ResourceType type,
                             std::vector<std::shared_ptr<contract::ResourceAbstract>> data);
    void notifyTreeObservers(contract::Subsystem subsystem, contract::ResourceType type,
                             std::vector<std::shared_ptr<contract::ResourceAbstract>> data);
    void notifyResourceObservers(contract::Subsystem subsystem, contract::ResourceType type, uint64_t resId,
                                 std::shared_ptr<contract::ResourceAbstract> resource);

public slots:
    // СЛОТЫ для подписок
    void onSubscribeToResourceList(contract::Subsystem subsystem,
                                   contract::ResourceType type,
                                   IDataObserver* observer);

    void onSubscribeToResourceTree(contract::Subsystem subsystem,
                                   contract::ResourceType type,
                                   IDataObserver* observer);

    void onSubscribeToResource(contract::Subsystem subsystem,
                               contract::ResourceType type,
                               uint64_t resId,
                               IDataObserver* observer);

    void onGetResource(contract::Subsystem subsystem,
                       contract::ResourceType type,
                       uint64_t resourceId,
                       IDataObserver* observer);

    // Другие слоты
    void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message);
    void onStartLoadResources(QByteArray userhash);
    void onSubsystemGenerate(contract::Subsystem subsystem,
                             contract::GenSubsystemType genType,
                             uint64_t genId,
                             bool created);
    void onCustomized();

signals:
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);
    void signalCutomize(QByteArray& userhash);
    void signalResourcesLoaded();
};

} // namespace uniter::data

#endif // DATAMANAGER_H
