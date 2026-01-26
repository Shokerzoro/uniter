
#ifndef ISUBSWDG_H
#define ISUBSWDG_H

#include "../messages/unitermessage.h"
#include <QWidget>
#include <cstdint>

namespace uniter::genwdg {


class ISubsWdg : public QWidget {
    Q_OBJECT

public:
    explicit ISubsWdg(messages::Subsystem subsystem,
                     messages::GenSubsystemType genType,
                     uint64_t genId,
                     QWidget* parent = nullptr);

    // TODO: удалить конструкторы

signals:
    // Проксирует сообщения вверх
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> message);

public slots:
    // Получает сообщения (от динамических виджетов)
    void onSendUniterMessage(std::shared_ptr<messages::UniterMessage> message);

private:
    messages::Subsystem subsystem;
    messages::GenSubsystemType genType;
    uint64_t genId;
};


} // genwdg

#endif // ISUBSWDG_H
