
#include "widgets_static/mainwindow.h"
#include "managers/appmanager.h"
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
    common::appfuncs::embed_main_exe_core_data();

    // Задаем переменные окружения
    common::appfuncs::AppEnviroment AEnviroment = common::appfuncs::get_env_data();
    common::appfuncs::set_env(AEnviroment);
    common::appfuncs::open_log(AEnviroment.logfile);

    // Слой виджетов
    uniter::staticwdg::MainWidget MWindow;

    // Слой данных

    // Управляющий слой
    uniter::managers::AppManager AManager;

    // Сетевой слой


    // Запуск FSM приложения

    MWindow.show();
    return app.exec();
}
