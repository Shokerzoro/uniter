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
    //Получаем переменные среды
    Path temp_dir, work_dir;
    QString app_name, parent_pid, main_exe, recover_exe, new_vers;
    appfuncs::get_env(temp_dir, work_dir, app_name, parent_pid, main_exe, recover_exe, new_vers);

    // Проверка существования путей
    if (!QDir(temp_dir).exists() || !QDir(work_dir).exists())
    {
        qDebug() << "Updater: wrong/unexisting working/temp dir paths";
        return 1;
    }
    //Добавить проверку наличия main.exe recover.exe
    // (!QFile::exists(main_exe) || !QFile::exists(recover_exe))
    if (!QFile::exists(main_exe))
    {
        qDebug() << "Updater: main.exe or recover.exe missing";
        return 2;
    }

    // Ждём завершения родителя
    upfuncs::wait_for_process_exit(parent_pid);

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
        upfuncs::get_update_data(temp_dir, work_dir, newdir_mapper, newfile_mapper, replace_mapper, delfile_vector, deldir_vector);

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
        QSettings settings("HKEY_CURRENT_USER\\Software\\" + app_name, QSettings::NativeFormat);
        settings.setValue("Version", new_vers);

        //Удаляем каталог temp_dir
        std::filesystem::remove_all(temp_dir);

        // Успешное выполнение
        qDebug() << "Updater: update succeses!";

    }
    catch (std::runtime_error & ex)
    {
        qDebug() << "Updater: update error! " << ex.what();
        //Тут по идее запуск recover.exe
        QProcess::startDetached(recover_exe);
        return 3;
    }
    catch (std::exception & ex)
    {
        qDebug() << "Updater: update error! " << ex.what();
        //Тут по идее запуск recover.exe
        QProcess::startDetached(recover_exe);
        return 4;
    }

    //Перезапуск основного бинарника main.exe
    if (!QProcess::startDetached(main_exe)) {
        qDebug() << "Updater: main_exe restart error";
        return 5;
    }

    return 0;
}



