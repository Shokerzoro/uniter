#include "widgets_static/mainwindow.h"
#include "widgets_static/status/updatecontainer.h"
#include "network/updaterworker.h"
#include "network/mainnetworker.h"

#include "../common/appfuncs.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Проверка уникальности приложения
    if(!appfuncs::is_single_instance()) return 0;

    // Задаем переменные окружения
    appfuncs::embed_main_exe_core_data();
    appfuncs::AppEnviroment app_env = appfuncs::get_env_data();
    appfuncs::set_env(app_env);
    appfuncs::open_log(app_env.logfile);

    //Создание основных ресурсов
    MainWindow MainWin;
    // Менеджер приложения
    // Сетевой класс

    MainWin.show();
    return app.exec();
}
