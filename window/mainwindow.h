#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "status/updatecontainer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    //Основные виджеты (которые получают сигналы от воркеров)
    UpdateContainer* update_container;

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
#endif // MAINWINDOW_H
