#include "mainwindow.h"
#include "status/updatecontainer.h"

#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    //Настройка наполнения
    this->setMinimumSize(800, 500);
    statusBar()->setStyleSheet("background-color: transparent;");
    statusBar()->setStyleSheet("QStatusBar::item {border: None;}");
    //statusBar()->setMaximumHeight(30);

    //Добавление виджетов
    update_container = new UpdateContainer();
    statusBar()->addPermanentWidget(update_container);

}


//Слоты для UpdateWorker
void MainWindow::UpWorkerNoUpdaterExe(void)
{

}

void MainWindow::UpWorkerNoRecoverExe(void)
{

}

void MainWindow::UpWorkerNoServerData(void)
{

}

void MainWindow::UpWorkerOnline(void)
{

}

void MainWindow::UpWorkerOffline(void)
{

}

void MainWindow::UpdateReady(void)
{

}
