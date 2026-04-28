#ifndef WORKAREA_H
#define WORKAREA_H

#include "../contract/unitermessage.h"
#include "../../../widgets_generative/subsystemtab.h"
#include <QWidget>
#include <QStackedLayout>
#include <map>
#include <memory>
#include <cstdint>

namespace uniter::staticwdg {


class WorkArea : public QWidget {
    Q_OBJECT

public:
    explicit WorkArea(QWidget* parent = nullptr);

    // Called by WorkWdg when adding a subsystem
    void addSubsystem(contract::Subsystem subsystem,
                      contract::GenSubsystem genType,
                      uint64_t genId,
                      int index);

    // Called by WorkWdg when a subsystem is removed
    void removeSubsystem(int index);

signals:
    // Proxies messages up into the network
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

public slots:
    // Receives a signal from StatusBar via WorkWdg
    void onSwitchTab(int index);

    // Receives messages from WorkWdg
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

private:
    // Stores subsystem widgets (index -> SubsWdg)
    std::map<int, genwdg::SubsystemTab*> subsystemWidgets;

    // StackedLayout for switching between tabs
    QStackedLayout* stackedLayout;
};


} // uniter::staticwdg

#endif // WORKAREA_H
