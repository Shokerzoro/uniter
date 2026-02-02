
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
                     contract::GenSubsystemType genType,
                     uint64_t genId,
                     QWidget* parent = nullptr);

    // TODO: удалить конструкторы

signals:
    // Проксирует сообщения вверх
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

public slots:
    // Получает сообщения (от динамических виджетов)
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

private:
    contract::Subsystem subsystem;
    contract::GenSubsystemType genType;
    uint64_t genId;
};


} // genwdg

#endif // ISUBSWDG_H
