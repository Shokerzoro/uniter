#include "window/mainwindow.h"
#include "common/appfuncs.h"
#include "workers/updaterworker.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow MainWin;
    MainWin.show();

    //Создание потока для проверки обновления
    //Этот поток будет отвечать за все взаимодействие
    QThread* NetThread = new QThread;

    //Подключаем воркера обновлений к сетевому потоку
    UpdaterWorker* UpWorker = new UpdaterWorker;
    UpWorker->moveToThread(NetThread);
    QObject::connect(NetThread, &QThread::started, UpWorker, &UpdaterWorker::StartRun);
    QObject::connect(NetThread, &QThread::finished, NetThread, &QThread::deleteLater);
    //Подключаем воркер к главному окну
    QObject::connect(UpWorker, SIGNAL(signalNoUpdaterExe), &MainWin,  SLOT(UpWorkerNoUpdaterExe));
    QObject::connect(UpWorker, SIGNAL(signalNoRecoverExe), &MainWin,  SLOT(UpWorkerNoRecoverExe));
    QObject::connect(UpWorker, SIGNAL(signalNoServerData), &MainWin,  SLOT(UpWorkerNoServerData));
    QObject::connect(UpWorker, SIGNAL(signalOnline), &MainWin,  SLOT(UpWorkerOnline));
    QObject::connect(UpWorker, SIGNAL(signalOffline), &MainWin,  SLOT(UpWorkerOffline));
    QObject::connect(UpWorker, SIGNAL(signalUpdateReady), &MainWin,  SLOT(UpdateReady));
    QObject::connect(&MainWin, SIGNAL(signalMakeUpdates), UpWorker,  SLOT(MakeUpdates));
    QObject::connect(&MainWin, SIGNAL(signalRefuseUpdates), UpWorker,  SLOT(RefuseUpdates));

    //Тут подключение основного серверного воркера к сетевому потоку
    //Как-то нужно работать с сохранением данных
    //Запоминать логин пароль грубо говоря

    //Запускаем поток
    NetThread->start();
    return app.exec();
}
