#include "mainwindow.h"
#include "status/updatecontainer.h"

#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>

namespace uniter {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    //Настройка наполнения
    this->setMinimumSize(800, 500);
    statusBar()->setStyleSheet("background-color: transparent;");
    statusBar()->setStyleSheet("QStatusBar::item {border: None;}");
    //statusBar()->setMaximumHeight(30);

    //Добавление виджетов
    //Статус бар
    update_container = new UpdateContainer();
    statusBar()->addPermanentWidget(update_container);

    //Центральный виджет
    QWidget* central_widget = new QWidget();
    this->setCentralWidget(central_widget);
    QVBoxLayout* central_widget_layout = new QVBoxLayout(central_widget);
    central_label = new QLabel();
    central_label->setStyleSheet("font-size: 24pt;");
    central_widget_layout->addWidget(central_label, 0, Qt::AlignCenter);
    central_label->setText("No version detacted");


}

} //namespace uniter
