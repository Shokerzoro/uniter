// updater/main.cpp
// Собирается в отдельный бинарник

#include <QCoreApplication>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QString>
#include <string>
#include <map>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <windows.h>

#include "upfuncs.h"
#include "../common/appfuncs.h"

using Path = std::filesystem::path;
using PathMapper = std::map<std::filesystem::path, std::filesystem::path>;
using PathVector = std::vector<std::filesystem::path>;


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    //Получаем переменные среды и открываем лог файл
    appfuncs::AppEnviroment app_env = appfuncs::read_env();
    appfuncs::open_log(app_env.logfile);
    appfuncs::write_log("Updater: starts routine");
    appfuncs::write_log(QString("Updater: performing update from %1 to %2")
                            .arg(app_env.version, app_env.new_version));

    // Проверка существования путей
    if (!QDir(app_env.temp_dir).exists() || !QDir(app_env.working_dir).exists())
    {
        appfuncs::write_log("Updater: wrong/unexisting working/temp dir paths");
        return 1;
    }
    //Добавить проверку наличия main.exe recover.exe
    // (!QFile::exists(main_exe) || !QFile::exists(recover_exe))
    if (!QFile::exists(app_env.main_exe))
    {
        appfuncs::write_log("Updater: main.exe or recover.exe missing");
        return 2;
    }

    // Ждём завершения родителя
    upfuncs::wait_for_process_exit(app_env.pid);

    //Контейнер для путей новых каталогов (пара temp_dir_path, target_dir_path)
    PathMapper newdir_mapper;
    //Контейнер для путей новых файлов (пара temp_file_path, target_file_path)
    PathMapper newfile_mapper;
    //Для путей файлов, подлежащих замене (пара actual_file_path, old_file_path)
    PathMapper replace_mapper;
    //Для путей файлов, подлежащих удалению
    PathVector delfile_vector;
    //Для путей каталогов, подлежищих удалению
    PathVector deldir_vector;

    try { //Блок вызова основных функций апдейтера

        //Получаем данные для обновлений
        upfuncs::get_update_data(app_env.temp_dir, app_env.working_dir,
                                 newdir_mapper, newfile_mapper, replace_mapper,
                                 delfile_vector, deldir_vector);

        //Вызываем функцию копирования новых каталогов
        upfuncs::addnew_dirs(newdir_mapper);
        //копирования новых файлов
        upfuncs::addnew_files(newfile_mapper);
        //замены старых файлов на новые replace_mapper
        upfuncs::replace_files(replace_mapper);
        //удаления старых версий (помечены при замене) delfile_vector
        upfuncs::delete_files(delfile_vector);
        //удаления элементов, подлежащих удалению deldir_vector
        upfuncs::delete_dirs(deldir_vector);

        //Заменяем ключ реестра "HKEY_CURRENT_USER\\Software\\" + application_name
        QSettings settings(QString("HKEY_CURRENT_USER\\Software\\%1").arg(app_env.appname), QSettings::NativeFormat);
        settings.setValue("Version", app_env.new_version);

        //Удаляем каталог temp_dir
        std::filesystem::remove_all(app_env.temp_dir.toStdString());

        // Успешное выполнение
        appfuncs::write_log("Updater: update succeses!");
        appfuncs::log_time();
    }
    catch (std::runtime_error & ex)
    {
        appfuncs::write_log(QString("Updater: update error! ") + QString::fromUtf8(ex.what()));
        //Тут по идее запуск recover.exe
        //QProcess::startDetached(app_env.recover_exe);
        return 3;
    }
    catch (std::exception & ex)
    {
        appfuncs::write_log(QString("Updater: update error! ") + QString::fromUtf8(ex.what()));
        //Тут по идее запуск recover.exe
        //QProcess::startDetached(app_env.recover_exe);
        return 4;
    }

    //Перезапуск основного бинарника main.exe
    if (!QProcess::startDetached(app_env.main_exe))
    {
        appfuncs::write_log("Updater: main_exe restart error");
        return 5;
    }

    return 0;
}
