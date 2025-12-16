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

namespace appfuncs {

// --- Логирование ---

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

// --- Работа с реестром ---

// Для тестирования на тестовом проекте
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

// Получить текущую версию из реестра (основываясь на данных ядра приложения)
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

// Изменяем версию в реестре, основываясь на переменных среды (вызывается из дочернего процесса)
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

// --- Работа с ядром приложения ---

void embed_main_exe_core_data()
{
    QString exePath = QCoreApplication::applicationFilePath();
    QFileInfo exeInfo(exePath);
    QString app_name = exeInfo.baseName();
    QCoreApplication::setApplicationName(app_name);

    qDebug() << "Имя приложения: " << app_name;

    QString version = get_reg_version();
    QCoreApplication::setApplicationVersion(version);

    qDebug() << "Текущая версия: " << version;
}

bool is_single_instance(void)
{
    // Если бы была винда, то можно было бы создать именнованый мьютекс
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
        throw std::runtime_error("appfuncs: getFileSHA256: не удалось открыть файл для чтения при хэшировании");

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
        hash.addData(file.read(4096));

    return hash.result().toHex();
}

// --- Работа с переменными среды ---

//Получаем переменные среды (для редактирования или сразу вызов следующей функции)
AppEnviroment get_env_data(void)
{
    AppEnviroment app_env;

    // temp_dir - локальная папка данных приложения
    app_env.temp_dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).absolutePath();

    // working_dir - каталог, где лежит исполняемый файл
    app_env.working_dir = QCoreApplication::applicationDirPath();

    // основной исполняемый файл
    app_env.main_exe = QCoreApplication::applicationFilePath();

    // относительные пути от working_dir
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

//Устанавливаем перед созданием процесса
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

//Читаем переменные среды (в начале нового процесса)
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

//Логирование переменных среды
void write_env()
{
    // Получаем переменные окружения через Qt
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

    // Записываем в лог с проверкой, чтобы не писать пустые строки
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
