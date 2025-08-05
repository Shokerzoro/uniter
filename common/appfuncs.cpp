#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QSettings>
#include <QString>
#include <QDir>
#include <filesystem>

namespace appfuncs {

using Path = std::filesystem::path;

//Устанавливаем переменные среды
QProcessEnvironment set_env(QString & new_version) noexcept
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    env.insert("NEW_VERSION", new_version);
    env.insert("TEMP_DIR", QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).absolutePath());
    env.insert("WORKING_DIR", QCoreApplication::applicationDirPath());
    env.insert("APP_NAME", QCoreApplication::applicationName());
    env.insert("MAIN_EXE", QCoreApplication::applicationFilePath());
    env.insert("RECOVER_EXE", QDir(QCoreApplication::applicationDirPath()).filePath("bin/recover.exe"));
    env.insert("PARENT_PID", QString::number(QCoreApplication::applicationPid()));

    return env;
}

//Получаем переменные среды
void get_env(Path & temp_dir, Path & working_dir, QString & app_name, QString & parent_pid,
                        QString & main_exe, QString & recover_exe, QString & new_vers) noexcept
{
    //которые отмечают то, какие файлы/каталоги подлежат удалению
    temp_dir = std::filesystem::path(QString::fromUtf8(qgetenv("TEMP_DIR")).toStdWString());
    //Каталог, который содержит в корне main.exe и все необходимые ресурсы
    working_dir = std::filesystem::path(QString::fromUtf8(qgetenv("WORKING_DIR")).toStdWString());
    //Имя из оригинального exe чтобы не было лишних ошибок
    app_name = qgetenv("APP_NAME");
    //Бинарник основного приложения
    main_exe  = qgetenv("MAIN_EXE");
    //Бинарник, который выполняет отмену изменений в случае неудачи (какие-то серьезные ошибки)
    recover_exe  = qgetenv("RECOVER_EXE");
    //Новая версия, которая должна быть помещена в реестр после завершения обновлений
    new_vers = qgetenv("NEW_VERSION");
    //PID основного процесса, из которого был запущен текущий
    parent_pid = qgetenv("PARENT_PID");
}


} //namespace appfuncs
