#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "../messages/unitermessage.h"
#include "../resources/resourceabstract.h"
#include "../widgets_generative/generativetab.h"
#include <QObject>
#include <QCryptographicHash>
#include <memory>


namespace uniter::data {


class DataManager : public QObject
{
    Q_OBJECT
public:
    DataManager(QObject* parent = nullptr);

private:
    // State Management
    enum class DBState { IDLE, LOADING, LOADED, ERROR };
    DBState state = DBState::IDLE;
    void setState(DBState newState);

    // Список обзерверов
    std::vector<std::weak_ptr<genwdg::ISubsWdg>> treeObservers;
    std::vector<std::weak_ptr<genwdg::ISubsWdg>> listObservers;
    std::vector<std::weak_ptr<genwdg::ISubsWdg>> resourceObservers;

public slots:
    void onRecvUniterMessage(std::shared_ptr<messages::UniterMessage> message);
    void onStartLoadResources(QCryptographicHash& userhash); // От менеджера приложения
    void onSubsystemGenerate(messages::Subsystem subsystem, messages::GenSubsystemType genType, uint64_t genId, bool created);
    void onCustomized(); // От менеджера конфигураций

    // Подписки на ресурсы
    void onSubscribeToResourceList(messages::Subsystem subsystem, messages::ResourceType);
    void onSubscribeToResourceTree(messages::Subsystem subsystem, messages::ResourceType);
    void onSubscribeToResource(messages::Subsystem subsystem, messages::ResourceType, uint64_t resId);
    // Получить ресурс
    std::shared_ptr<resources::ResourceAbstract> getResource(messages::Subsystem subsystem,
                                                  messages::ResourceType type,
                                                  uint64_t resourceId);
signals:
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> message);
    void signalCutomize(QCryptographicHash& userhash); // Для менеджера конфигураций
};

} // data

#endif // DATAMANAGER_H
