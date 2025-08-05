#ifndef UPDATECONTAINER_H
#define UPDATECONTAINER_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include "update_indicator.h"

class UpdateContainer : public QWidget
{
    Q_OBJECT
public:
    explicit UpdateContainer(QWidget *parent = nullptr);
    //Состояние
    enum Status { ONLINE, OFFLINE, SOMEUPDATE };

    //Виджеты
    UpdateIndicator* update_indicator;
    QLabel* update_label;
    QToolButton* agree_button;
    QToolButton* refuse_button;

signals:
    //Для UpdateWorker
    void signalMakeUpdates();
    void signalRefuseUpdates();

public slots:
    //От UpdateWorker
    void UpWorkerNoUpdaterExe(void);
    void UpWorkerNoRecoverExe(void);
    void UpWorkerNoServerData(void);
    void UpWorkerOnline(void);
    void UpWorkerOffline(void);
    void UpdateReady(void);
};

#endif // UPDATECONTAINER_H
