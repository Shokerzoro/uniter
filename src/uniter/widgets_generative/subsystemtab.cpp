#include "subsystemtab.h"
#include "../control/uimanager.h"
#include <QLabel>
#include <QVBoxLayout>

namespace uniter::genwdg {

SubsystemTab::SubsystemTab(contract::Subsystem subsystem,
                   contract::GenSubsystemType genType,
                   uint64_t genId,
                   QWidget* parent)
    : QWidget(parent), subsystem(subsystem), genType(genType), genId(genId) {

    QString fullName;

    if (subsystem == contract::Subsystem::GENERATIVE) {
        switch (genType) {
        case contract::GenSubsystemType::INTERGATION:
            fullName = "Integration";
            break;
        case contract::GenSubsystemType::PRODUCTION:
            fullName = "Production";
            break;
        default:
            fullName = "Unknown";
            break;
        }
    } else {
        switch (subsystem) {
        case contract::Subsystem::DESIGN:
            fullName = "Design";
            break;
        case contract::Subsystem::MANAGER:
            fullName = "Manager";
            break;
        case contract::Subsystem::MATERIALS:
            fullName = "Materials";
            break;
        case contract::Subsystem::PURCHASES:
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
    auto settings = control::UIManager::instance();
    settings->applyGenerativeTabSettings(this);
}

void SubsystemTab::onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message) {
    message->subsystem = subsystem;
    emit signalSendUniterMessage(message);
}

} // namespace uniter::genwdg
