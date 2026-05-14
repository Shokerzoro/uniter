#ifndef WORKWIDGET_H
#define WORKWIDGET_H

#include "../../contract/unitermessage.h"
#include "./workbar/workbar.h"
#include "./workarea/workarea.h"
#include <QWidget>
#include <map>
#include <memory>
#include <cstdint>
#include <optional>

namespace staticwdg {

class WorkWdg : public QWidget {
    Q_OBJECT

public:
    explicit WorkWdg(QWidget* parent = nullptr);

public slots:
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);
    void onSubsystemAdded(contract::Subsystem subsystem,
                          std::optional<uint64_t> subsystemInstanceId,
                          bool created);

signals:
    // Снизу (от виджетов)
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);
private:
    // Виджеты
    WorkBar* workbar;
    WorkArea* workArea;

    // Вектор активных подсистем
    struct ActiveSubsystem {
        ActiveSubsystem(contract::Subsystem subsystem_,
                        std::optional<uint64_t> subsystemInstanceId_)
            : subsystem{subsystem_}
            , subsystemInstanceId{subsystemInstanceId_}
        {}
        contract::Subsystem subsystem;
        std::optional<uint64_t> subsystemInstanceId = std::nullopt;
    };

    int nextIndex = 0;
    std::map<int, ActiveSubsystem> indexToSubsystem;


    // Приватные методы
    bool findIndex(contract::Subsystem subsystem,
                   std::optional<uint64_t> subsystemInstanceId,
                   int& outIndex) const;
    void addSubsystem(contract::Subsystem subsystem,
                      std::optional<uint64_t> subsystemInstanceId);
    void removeSubsystem(contract::Subsystem subsystem,
                         std::optional<uint64_t> subsystemInstanceId);
};

} // staticwdg

#endif // WORKWIDGET_H
