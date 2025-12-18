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
    if(!common::appfuncs::is_single_instance()) return 0;

    // Задаем переменные окружения
    common::appfuncs::embed_main_exe_core_data();
    common::appfuncs::AppEnviroment app_env = common::appfuncs::get_env_data();
    common::appfuncs::set_env(app_env);
    common::appfuncs::open_log(app_env.logfile);

    //Создание основных ресурсов
    uniter::MainWindow MainWin;
    // Менеджер приложения
    // Сетевой класс

    MainWin.show();
    return app.exec();
}
