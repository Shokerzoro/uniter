#include "generativetab.h"
#include "../managers/uimanager.h"
#include <QLabel>
#include <QVBoxLayout>

namespace uniter::genwdg {

ISubsWdg::ISubsWdg(messages::Subsystem subsystem,
                   messages::GenSubsystemType genType,
                   uint64_t genId,
                   QWidget* parent)
    : QWidget(parent), subsystem(subsystem), genType(genType), genId(genId) {

    QString fullName;

    if (subsystem == messages::Subsystem::GENERATIVE) {
        switch (genType) {
        case messages::GenSubsystemType::INTERGATION:
            fullName = "Integration";
            break;
        case messages::GenSubsystemType::PRODUCTION:
            fullName = "Production";
            break;
        default:
            fullName = "Unknown";
            break;
        }
    } else {
        switch (subsystem) {
        case messages::Subsystem::DESIGN:
            fullName = "Design";
            break;
        case messages::Subsystem::MANAGER:
            fullName = "Manager";
            break;
        case messages::Subsystem::MATERIALS:
            fullName = "Materials";
            break;
        case messages::Subsystem::PURCHASES:
            fullName = "Purchases";
            break;
        default:
            fullName = "Unknown";
            break;
        }
    }

    // Создаем QLabel с полным текстом + "subsystem"
    QLabel* nameLabel = new QLabel(fullName + " subsystem", this);
    nameLabel->setAlignment(Qt::AlignCenter);

    // Создаем layout для центрирования
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(nameLabel);
    setLayout(layout);

    // Применяем настройки UIManager
    auto settings = managers::UIManager::instance();
    settings->applyGenerativeTabSettings(this);
}

void ISubsWdg::onSendUniterMessage(std::shared_ptr<messages::UniterMessage> message) {
    message->subsystem = subsystem;
    emit signalSendUniterMessage(message);
}

} // namespace uniter::genwdg
