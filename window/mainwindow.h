#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "status/updatecontainer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    //Основные виджеты (которые получают сигналы от воркеров)
    UpdateContainer* update_container;
    QLabel* central_label;

signals:


public slots:


};
#endif // MAINWINDOW_H
