
#ifndef ISUBSWDG_H
#define ISUBSWDG_H

#include "../contract/unitermessage.h"
#include <cstdint>
#include <QWidget>

namespace uniter::genwdg {


class SubsystemTab : public QWidget {
    Q_OBJECT

public:
    explicit SubsystemTab(contract::Subsystem subsystem,
                     contract::GenSubsystem genType,
                     uint64_t genId,
                     QWidget* parent = nullptr);

    // TODO: remove constructors

signals:
    // Proxies messages up
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

public slots:
    // Receives messages (from dynamic widgets)
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

private:
    contract::Subsystem subsystem;
    contract::GenSubsystem genType;
    uint64_t genId;
};


} // genwdg

#endif // ISUBSWDG_H
