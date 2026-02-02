
#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "../messages/unitermessage.h"
#include "../resources/resourceabstract.h"
#include "../widgets_generative/generativetab.h"
#include <QObject>
#include <QByteArray>
#include <memory>


namespace uniter::data {

class DataManager : public QObject
{
    Q_OBJECT

private:
    // Приватный конструктор для синглтона
    DataManager();

    // Запрет копирования и перемещения
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;
    DataManager(DataManager&&) = delete;
    DataManager& operator=(DataManager&&) = delete;

    // State Management
    enum class DBState { IDLE, LOADING, LOADED, ERROR };
    DBState state = DBState::IDLE;
    void setState(DBState newState);

    // Список обзерверов
    std::vector<std::weak_ptr<genwdg::ISubsWdg>> treeObservers;
    std::vector<std::weak_ptr<genwdg::ISubsWdg>> listObservers;
    std::vector<std::weak_ptr<genwdg::ISubsWdg>> resourceObservers;

public:
    // Публичный статический метод получения экземпляра
    static DataManager* instance();

    ~DataManager() override = default;

public slots:
    void onRecvUniterMessage(std::shared_ptr<messages::UniterMessage> message);
    void onStartLoadResources(QByteArray userhash); // От менеджера приложения
    void onSubsystemGenerate(messages::Subsystem subsystem,
                             messages::GenSubsystemType genType,
                             uint64_t genId,
                             bool created);
    void onCustomized(); // От менеджера конфигураций

    // Подписки на ресурсы (доп. аргумент – ISubsWdg)
    void onSubscribeToResourceList(messages::Subsystem subsystem,
                                   messages::ResourceType type,
                                   std::shared_ptr<genwdg::ISubsWdg> observer);
    void onSubscribeToResourceTree(messages::Subsystem subsystem,
                                   messages::ResourceType type,
                                   std::shared_ptr<genwdg::ISubsWdg> observer);
    void onSubscribeToResource(messages::Subsystem subsystem,
                               messages::ResourceType type,
                               uint64_t resId,
                               std::shared_ptr<genwdg::ISubsWdg> observer);

    // Получить ресурс
    void onGetResource(messages::Subsystem subsystem,
                       messages::ResourceType type,
                       uint64_t resourceId,
                       std::shared_ptr<genwdg::ISubsWdg> observer);

signals:
    void signalCutomize(QByteArray& userhash); // Для менеджера конфигураций
    void signalResourcesLoaded(); // Менеджеру приложения
};

} // namespace uniter::data

#endif // DATAMANAGER_H
