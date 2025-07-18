#include "mainwindow.h"
#include "versions/version.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    embed_meta(app);

    //

    MainWindow w;
    w.show();
    return app.exec();
}
