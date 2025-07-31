#include "mainwindow.h"
#include "versions/version.h"
#include "net/updaterworker.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    embed_meta();

    MainWindow mainwin;
    mainwin.show();

    //Создание потока для проверки обновления
    //Этот поток будет отвечать за все взаимодействие
    QThread* NetThread = new QThread;

    //Подключаем воркера обновлений к сетевому потоку
    UpdaterWorker* UpWorker = new UpdaterWorker;
    UpWorker->moveToThread(NetThread);
    QObject::connect(NetThread, &QThread::started, UpWorker, &UpdaterWorker::StartRun);
    QObject::connect(NetThread, &QThread::finished, NetThread, &QThread::deleteLater);

    //Тут подключение основного серверного воркера к сетевому потоку

    //Запускаем поток
    NetThread->start();
    return app.exec();
}
