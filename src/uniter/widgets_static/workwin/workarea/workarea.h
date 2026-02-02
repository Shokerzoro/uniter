#ifndef WORKAREA_H
#define WORKAREA_H

#include "../../../contract/unitermessage.h"
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

    // Вызывается WorkWdg при добавлении подсистемы
    void addSubsystem(contract::Subsystem subsystem,
                      contract::GenSubsystemType genType,
                      uint64_t genId,
                      int index);

    // Вызывается WorkWdg при удалении подсистемы
    void removeSubsystem(int index);

signals:
    // Проксирует сообщения вверх в сеть
    void signalSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

public slots:
    // Получает сигнал от StatusBar через WorkWdg
    void onSwitchTab(int index);

    // Получает сообщения из WorkWdg
    void onSendUniterMessage(std::shared_ptr<contract::UniterMessage> message);

private:
    // Хранит виджеты подсистем (index -> SubsWdg)
    std::map<int, genwdg::SubsystemTab*> subsystemWidgets;

    // StackedLayout для переключения между вкладками
    QStackedLayout* stackedLayout;
};


} // uniter::staticwdg

#endif // WORKAREA_H
