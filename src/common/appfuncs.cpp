#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QSettings>
#include <QString>
#include <QDir>
#include <QDebug>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <QDateTime>
#include <QFileInfo>
#include <QCryptographicHash>
#include <windows.h>

#include "appfuncs.h"

using Path = std::filesystem::path;

namespace common {
namespace appfuncs {

// ---Logging ---

static std::ofstream log_stream;

void open_log(const QString & logfilepath)
{
    Path path = Path(logfilepath.toStdWString()); // QString → Path (std::filesystem::path)

    std::filesystem::create_directories(path.parent_path());
    log_stream.open(path, std::ios::app | std::ios::out);

    if (log_stream.is_open())
    {
        log_stream << std::endl;
        log_stream << "   ### Log opened at ";
        log_stream << QDateTime::currentDateTimeUtc()
                          .toString("yyyy-MM-dd hh:mm:ss")
                          .toStdString();
        log_stream << std::endl;
    }
    else
        throw std::runtime_error("Cannot open log file");

    qDebug() << "Log opened:" << QString::fromStdWString(path.wstring());
}


void write_log(const QString & msg)
{
    if(log_stream.is_open())
    {
        log_stream << msg.toStdString();
        log_stream << '\n';
        log_stream.flush();
    }

    qDebug() << "Logging: " << msg;
}

void log_time(void)
{
    if (log_stream.is_open())
    {
        log_stream << std::endl;
        log_stream << " Timed: ";
        log_stream << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        log_stream << std::endl;
    }
}

void close_log()
{
    if (log_stream.is_open())
    {
        log_stream << std::endl;
        log_stream << "   ### Log closed at ";
        log_stream << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        log_stream << std::endl;
        log_stream.close();
    }

    qDebug() << "Log closed!";
}

// --- Working with the registry ---

// For testing on a test project
void add_reg_key(void)
{
    QString exePath = QCoreApplication::applicationFilePath();
    QFileInfo exeInfo(exePath);
    QString app_name = exeInfo.baseName();

    if (app_name.isEmpty())
        throw std::runtime_error("Cannot determine application name");

    QString reg_base = R"(HKEY_CURRENT_USER\Software\)";
    QString reg_key = reg_base + app_name;

    qDebug() << "Adding reg key: " << reg_key;

    QSettings reg(reg_key, QSettings::NativeFormat);
    reg.setValue("Version", "0.0.0");
    reg.sync();
}

// Get the current version from the registry (based on application core data)
QString get_reg_version(void)
{
    QString app_name = QCoreApplication::applicationName();
    if (app_name.isEmpty())
        throw std::runtime_error("Application name is not set");

    QString reg_base = R"(HKEY_CURRENT_USER\Software\)";
    QString reg_key = reg_base + app_name;

    QSettings reg(reg_key, QSettings::NativeFormat);
    QVariant version = reg.value("Version");

    if (!version.isValid() || version.toString().isEmpty())
        throw std::runtime_error("App is not installed or version key is missing");

    return version.toString();
}

// Change the version in the registry based on environment variables (called from a child process)
void set_reg_new_version(void)
{
    QByteArray app_name_env = qgetenv("APP_NAME");
    QByteArray new_version_env = qgetenv("NEW_VERSION");

    qDebug() << "Setting in " << app_name_env.constData() << " new version " << new_version_env;

    if (app_name_env.isEmpty())
        throw std::runtime_error("APP_NAME environment variable is not set");

    if (new_version_env.isEmpty())
        throw std::runtime_error("NEW_VERSION environment variable is not set");

    QString app_name = QString::fromUtf8(app_name_env);
    QString new_version = QString::fromUtf8(new_version_env);

    QString reg_base = R"(HKEY_CURRENT_USER\Software\)";
    QString reg_key = reg_base + app_name;

    QSettings reg(reg_key, QSettings::NativeFormat);
    reg.setValue("Version", new_version);
    reg.sync();

    qDebug() << "Reg changed!";
}

// --- Working with the application core ---

void embed_main_exe_core_data()
{
    QString exePath = QCoreApplication::applicationFilePath();
    QFileInfo exeInfo(exePath);
    QString app_name = exeInfo.baseName();
    QCoreApplication::setApplicationName(app_name);

    qDebug() << "Application name: " << app_name;

    QString version;
    try {
        version = get_reg_version();
    } catch (const std::runtime_error &) {
        // First launch without installation - create a key with the default version
        qDebug() << "Version was not found in the registry; creating entry 0.0.0";
        QString reg_key = R"(HKEY_CURRENT_USER\Software\)" + app_name;
        QSettings reg(reg_key, QSettings::NativeFormat);
        reg.setValue("Version", "0.0.0");
        reg.sync();
        version = "0.0.0";
    }

    QCoreApplication::setApplicationVersion(version);
    qDebug() << "Current version: " << version;
}

bool is_single_instance(void)
{
    // If you had Windows, you could create a named mutex
    HANDLE UniqueSem = CreateSemaphoreW(nullptr, 1, 1, L"UniterApp");

    if(!UniqueSem)
    {
        qDebug() << "appfuncs: is_single_instance: error semaphore creating!";
        return false;
    }

    DWORD WaitResult = WaitForSingleObject(UniqueSem, 0);
    if(WaitResult == WAIT_OBJECT_0)
    {
        qDebug() << "appfuncs: is_single_instance: first instance of Uniter starts!";
        return true;
    }
    else
    {
        qDebug() << "appfuncs: is_single_instance: Uniter already runs!";
        return false;
    }
}

QString getFileSHA256(QFile & file)
{
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("appfuncs: getFileSHA256: failed to open file for reading while hashing");

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
        hash.addData(file.read(4096));

    return hash.result().toHex();
}

// --- Working with environment variables ---

//We get environment variables (for editing or immediately calling the next function)
AppEnviroment get_env_data(void)
{
    AppEnviroment app_env;

    // temp_dir - local application data folder
    app_env.temp_dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).absolutePath();

    // working_dir - directory where the executable file is located
    app_env.working_dir = QCoreApplication::applicationDirPath();

    // main executable
    app_env.main_exe = QCoreApplication::applicationFilePath();

    // relative paths from working_dir
    app_env.recover_exe = QDir(app_env.working_dir).filePath("bin/recover.exe");
    app_env.updater_exe = QDir(app_env.working_dir).filePath("bin/updater.exe");
    app_env.logfile = QDir(app_env.working_dir).filePath("etc/log.txt");
    app_env.config = QDir(app_env.working_dir).filePath("config/config.ini");

    app_env.appname = QCoreApplication::applicationName();
    app_env.pid = QString::number(QCoreApplication::applicationPid());
    app_env.version = QCoreApplication::applicationVersion();
    app_env.new_version = QCoreApplication::applicationVersion();

    return app_env;
}

//Install before creating a process
void set_env(const AppEnviroment & app_env)
{
    qputenv("TEMP_DIR", app_env.temp_dir.toUtf8());
    qputenv("WORKING_DIR", app_env.working_dir.toUtf8());
    qputenv("APP_NAME", app_env.appname.toUtf8());
    qputenv("MAIN_EXE", app_env.main_exe.toUtf8());
    qputenv("RECOVER_EXE", app_env.recover_exe.toUtf8());
    qputenv("UPDATER_EXE", app_env.updater_exe.toUtf8());
    qputenv("LOGFILE", app_env.logfile.toUtf8());
    qputenv("CONFIG", app_env.config.toUtf8());
    qputenv("PARENT_PID", app_env.pid.toUtf8());
    qputenv("VERSION", app_env.version.toUtf8());
    qputenv("NEW_VERSION", app_env.new_version.toUtf8());
}

//Reading environment variables (at the start of a new process)
AppEnviroment read_env(void)
{
    AppEnviroment app_env;

    app_env.temp_dir = QString::fromUtf8(qgetenv("TEMP_DIR"));
    app_env.working_dir = QString::fromUtf8(qgetenv("WORKING_DIR"));
    app_env.appname = QString::fromUtf8(qgetenv("APP_NAME"));
    app_env.main_exe = QString::fromUtf8(qgetenv("MAIN_EXE"));
    app_env.recover_exe = QString::fromUtf8(qgetenv("RECOVER_EXE"));
    app_env.updater_exe = QString::fromUtf8(qgetenv("UPDATER_EXE"));
    app_env.logfile = QString::fromUtf8(qgetenv("LOGFILE"));
    app_env.config = QString::fromUtf8(qgetenv("CONFIG"));
    app_env.version = QString::fromUtf8(qgetenv("VERSION"));
    app_env.new_version = QString::fromUtf8(qgetenv("NEW_VERSION"));
    app_env.pid = QString::fromUtf8(qgetenv("PARENT_PID"));

    return app_env;
}

//Logging environment variables
void write_env()
{
    // Getting environment variables through Qt
    QString temp_dir = qEnvironmentVariable("TEMP_DIR");
    QString working_dir = qEnvironmentVariable("WORKING_DIR");
    QString appname = qEnvironmentVariable("APP_NAME");
    QString main_exe = qEnvironmentVariable("MAIN_EXE");
    QString recover_exe = qEnvironmentVariable("RECOVER_EXE");
    QString updater_exe = qEnvironmentVariable("UPDATER_EXE");
    QString logfile = qEnvironmentVariable("LOGFILE");
    QString config = qEnvironmentVariable("CONFIG");
    QString version = qEnvironmentVariable("VERSION");
    QString new_version = qEnvironmentVariable("NEW_VERSION");
    QString pid = qEnvironmentVariable("PARENT_PID");

    // We write to the log with a check so as not to write empty lines
    auto write_if_not_empty = [](const QString &name, const QString &value){
        write_log(name + " = " + (value.isEmpty() ? "<not set>" : value));
    };

    write_if_not_empty("TEMP_DIR", temp_dir);
    write_if_not_empty("WORKING_DIR", working_dir);
    write_if_not_empty("APP_NAME", appname);
    write_if_not_empty("MAIN_EXE", main_exe);
    write_if_not_empty("RECOVER_EXE", recover_exe);
    write_if_not_empty("UPDATER_EXE", updater_exe);
    write_if_not_empty("LOGFILE", logfile);
    write_if_not_empty("CONFIG", config);
    write_if_not_empty("VERSION", version);
    write_if_not_empty("NEW_VERSION", new_version);
    write_if_not_empty("PARENT_PID", pid);
}

} // namespace appfuncs
} // namespace common
