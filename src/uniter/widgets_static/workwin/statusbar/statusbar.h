#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "../../../messages/unitermessage.h"
#include "../../../widgets_generative/subsystemicon.h"
#include <QWidget>
#include <map>
#include <memory>

namespace uniter::staticwdg {


class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);

    void addSubsystem(messages::GenSubsystemType genType,
                      const QString& name,
                      int index);
    void removeSubsystem(int index);

signals:
    void signalSwitchTab(int index);
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> message);

public slots:
    void onSendUniterMessage(std::shared_ptr<messages::UniterMessage> message);

private slots:
    void onIconClicked(int index);

private:
    std::map<int, genwdg::SubsystemIcon*> subsystemIcons;
};


} // uniter::staticwdg


#endif // STATUSBAR_H
