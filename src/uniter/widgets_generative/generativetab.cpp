

#include "../messages/unitermessage.h"
#include "generativetab.h"
#include <QWidget>
#include <cstdint>

namespace uniter::genwdg {


ISubsWdg::ISubsWdg(messages::Subsystem subsystem,messages::GenSubsystemType genType,
                 uint64_t genId, QWidget* parent)
    : QWidget(parent), subsystem(subsystem), genType(genType), genId(genId) {

    // TODO: инициализация UI и применение uisettings
}

void ISubsWdg::onSendUniterMessage(std::shared_ptr<messages::UniterMessage> message) {

    message->subsystem = subsystem;
    emit signalSendUniterMessage(message);
}


} // genwdg
