// updater/main.cpp
// Collected into a separate binary

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

#include "typesheader.h"
#include "upfuncs.h"
#include "../common/appfuncs.h"

using namespace updater;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    //Get environment variables and open the log file
    common::appfuncs::AppEnviroment app_env = common::appfuncs::read_env();
    common::appfuncs::open_log(app_env.logfile);
    common::appfuncs::write_log("Updater: starts routine");
    common::appfuncs::write_log(QString("Updater: performing update from %1 to %2")
                            .arg(app_env.version, app_env.new_version));

    // Checking the existence of paths
    if (!QDir(app_env.temp_dir).exists() || !QDir(app_env.working_dir).exists())
    {
        common::appfuncs::write_log("Updater: wrong/unexisting working/temp dir paths");
        return 1;
    }
    //Add a check for the presence of main.exe recover.exe
    // (!QFile::exists(main_exe) || !QFile::exists(recover_exe))
    if (!QFile::exists(app_env.main_exe))
    {
        common::appfuncs::write_log("Updater: main.exe or recover.exe missing");
        return 2;
    }

    // Waiting for the parent to complete
    upfuncs::wait_for_process_exit(app_env.pid);

    //Container for new directory paths (temp_dir_path, target_dir_path pair)
    PathMapper newdir_mapper;
    //Container for new file paths (temp_file_path, target_file_path pair)
    PathMapper newfile_mapper;
    //For file paths to be replaced (pair actual_file_path, old_file_path)
    PathMapper replace_mapper;
    //For file paths to be deleted
    PathVector delfile_vector;
    //For directory paths to be deleted
    PathVector deldir_vector;

    try { // Block for calling the main functions of the updater

        //Getting data for updates
        upfuncs::get_update_data(app_env.temp_dir, app_env.working_dir,
                                 newdir_mapper, newfile_mapper, replace_mapper,
                                 delfile_vector, deldir_vector);

        //Calling the function to copy new directories
        upfuncs::addnew_dirs(newdir_mapper);
        //copying new files
        upfuncs::addnew_files(newfile_mapper);
        //replacing old files with new ones replace_mapper
        upfuncs::replace_files(replace_mapper);
        //deleting old versions (marked when replaced) delfile_vector
        upfuncs::delete_files(delfile_vector);
        //deleting elements to be deleted deldir_vector
        upfuncs::delete_dirs(deldir_vector);

        //Replace the registry key "HKEY_CURRENT_USER\\Software\\" + application_name
        QSettings settings(QString("HKEY_CURRENT_USER\\Software\\%1").arg(app_env.appname), QSettings::NativeFormat);
        settings.setValue("Version", app_env.new_version);

        //Removing the temp_dir directory
        std::filesystem::remove_all(app_env.temp_dir.toStdString());

        // Successful completion
        common::appfuncs::write_log("Updater: update succeses!");
        common::appfuncs::log_time();
    }
    catch (std::runtime_error & ex)
    {
        common::appfuncs::write_log(QString("Updater: update error! ") + QString::fromUtf8(ex.what()));
        //Here, in theory, launch recover.exe
        //QProcess::startDetached(app_env.recover_exe);
        return 3;
    }
    catch (std::exception & ex)
    {
        common::appfuncs::write_log(QString("Updater: update error! ") + QString::fromUtf8(ex.what()));
        //Here, in theory, launch recover.exe
        //QProcess::startDetached(app_env.recover_exe);
        return 4;
    }

    //Restarting the main binary main.exe
    if (!QProcess::startDetached(app_env.main_exe))
    {
        common::appfuncs::write_log("Updater: main_exe restart error");
        return 5;
    }

    return 0;
}
