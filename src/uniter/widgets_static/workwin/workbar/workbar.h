#ifndef WORKBAR_H
#define WORKBAR_H

#include "../../../contract/unitermessage.h"
#include "../../../widgets_generative/subsystemicon.h"
#include <QBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <map>
#include <memory>
#include <optional>
#include <cstdint>

namespace staticwdg {

class WorkBar : public QWidget {
    Q_OBJECT

public:
    explicit WorkBar(QWidget* parent = nullptr);

    void addSubsystem(contract::Subsystem subsystem,
                      std::optional<uint64_t> subsystemInstanceId,
                      int index);
    void removeSubsystem(int index);

signals:
    void signalSwitchTab(int index);
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);
    void signalLogout(); // Новый сигнал для LogOut

public slots:
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

private slots:
    void onIconClicked(int index);

private:
    QWidget* iconContainer;
    QVBoxLayout* iconLayout;
    QPushButton* logoutButton; // Кнопка LogOut
    std::map<int, genwdg::SubsystemIcon*> subsystemIcons;

protected:
    void paintEvent(QPaintEvent* event) override;
};

} // staticwdg

#endif // WORKBAR_H
