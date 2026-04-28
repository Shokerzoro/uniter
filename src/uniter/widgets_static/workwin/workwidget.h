#ifndef WORKWIDGET_H
#define WORKWIDGET_H

#include "../contract/unitermessage.h"
#include "./workbar/workbar.h"
#include "./workarea/workarea.h"
#include <QWidget>
#include <map>
#include <memory>
#include <cstdint>

namespace uniter::staticwdg {

class WorkWdg : public QWidget {
    Q_OBJECT

public:
    explicit WorkWdg(QWidget* parent = nullptr);

public slots:
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);
    void onSubsystemAdded(contract::Subsystem subsystem,
                          contract::GenSubsystem genType,
                          std::optional<uint64_t> genId,
                          bool created);

signals:
    // Bottom (from widgets)
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);
private:
    // Widgets
    WorkBar* workbar;
    WorkArea* workArea;

    // Vector of active subsystems
    struct ActiveSubsystem {
        ActiveSubsystem(contract::Subsystem subsystem_,
                        contract::GenSubsystem genType_,
                        uint64_t genId_)
            : subsystem{subsystem_}
            , genType{genType_}
            , genId{genId_}
        {}
        contract::Subsystem subsystem;
        contract::GenSubsystem genType;
        uint64_t genId = 0;
    };

    int nextIndex = 0;
    std::map<int, ActiveSubsystem> indexToSubsystem;


    // Private Methods
    bool findIndex(contract::Subsystem subsystem,
                   contract::GenSubsystem genType,
                   std::optional<uint64_t> genId,
                   int& outIndex) const;
    void addSubsystem(contract::Subsystem subsystem,
                      contract::GenSubsystem genType,
                      std::optional<uint64_t> genId);
    void removeSubsystem(contract::Subsystem subsystem,
                         contract::GenSubsystem genType,
                         std::optional<uint64_t> genId);
};

} // uniter::staticwdg

#endif // WORKWIDGET_H
