#ifndef WORKWIDGET_H
#define WORKWIDGET_H

#include "../../messages/unitermessage.h"
#include "./statusbar/statusbar.h"
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
    void onSendUniterMessage(std::shared_ptr<messages::UniterMessage> message);
    void onSubsystemAdded(messages::Subsystem subsystem,
                          messages::GenSubsystemType genType,
                          uint64_t genId,
                          bool created);

signals:
    // Снизу (от виджетов)
    void signalSendUniterMessage(std::shared_ptr<messages::UniterMessage> message);
private:
    // Виджеты
    StatusBar* statusBar;
    WorkArea* workArea;

    // Вектор активных подсистем
    struct ActiveSubsystem {
        ActiveSubsystem(messages::Subsystem subsystem_,
                        messages::GenSubsystemType genType_,
                        uint64_t genId_)
            : subsystem{subsystem_}
            , genType{genType_}
            , genId{genId_}
        {}
        messages::Subsystem subsystem;
        messages::GenSubsystemType genType;
        uint64_t genId = 0;
    };

    int nextIndex = 0;
    std::map<int, ActiveSubsystem> indexToSubsystem;


    // Приватные методы
    bool findIndex(messages::Subsystem subsystem,
                   messages::GenSubsystemType genType,
                   uint64_t genId,
                   int& outIndex) const;
    void addSubsystem(messages::Subsystem subsystem,
                      messages::GenSubsystemType genType,
                      uint64_t genId);
    void removeSubsystem(messages::Subsystem subsystem,
                         messages::GenSubsystemType genType,
                         uint64_t genId);
};

} // uniter::staticwdg

#endif // WORKWIDGET_H
