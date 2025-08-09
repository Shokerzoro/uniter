#include "window/mainwindow.h"
#include "window/status/updatecontainer.h"
#include "common/appfuncs.h"
#include "workers/updaterworker.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    appfuncs::embed_meta();

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
    QObject::connect(UpWorker, &UpdaterWorker::signalNoUpdaterExe, MainWin.update_container, &UpdateContainer::UpWorkerNoUpdaterExe);
    QObject::connect(UpWorker, &UpdaterWorker::signalNoRecoverExe, MainWin.update_container,  &UpdateContainer::UpWorkerNoRecoverExe);
    QObject::connect(UpWorker, &UpdaterWorker::signalNoServerData, MainWin.update_container,  &UpdateContainer::UpWorkerNoServerData);
    QObject::connect(UpWorker, &UpdaterWorker::signalOnline, MainWin.update_container,  &UpdateContainer::UpWorkerOnline);
    QObject::connect(UpWorker, &UpdaterWorker::signalOffline, MainWin.update_container,  &UpdateContainer::UpWorkerOffline);
    QObject::connect(UpWorker, &UpdaterWorker::signalUpdateReady, MainWin.update_container,  &UpdateContainer::UpdateReady);
    QObject::connect(MainWin.update_container, &UpdateContainer::signalMakeUpdates, UpWorker,  &UpdaterWorker::MakeUpdates);
    QObject::connect(MainWin.update_container, &UpdateContainer::signalRefuseUpdates, UpWorker,  &UpdaterWorker::RefuseUpdates);
    QObject::connect(UpWorker, &UpdaterWorker::signalSetVersion, MainWin.central_label, &QLabel::setText);


    //Тут подключение основного серверного воркера к сетевому потоку
    //Как-то нужно работать с сохранением данных
    //Запоминать логин пароль грубо говоря

    //Запускаем поток
    NetThread->start();
    return app.exec();
}
