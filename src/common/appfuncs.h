#ifndef APPFUNCS_H
#define APPFUNCS_H

#include <QProcessEnvironment>
#include <QString>
#include <QFile>
#include <filesystem>

namespace common {
namespace appfuncs {

using Path = std::filesystem::path;

// --- Логирование ---

//Открывает log_stream
extern void open_log(const QString & logfilepath);
//Пишет в log_stream
extern void write_log(const QString & msg);
//Пишет время и дату в log_stream
extern void log_time(void);
//Закрываем лог
extern void close_log(void);

// --- Работа с реестром ---

extern void add_reg_key(void); //Добавить запись, только для тестирования остального
extern QString get_reg_version(void); //Получить текущую версию
extern void set_reg_new_version(void); //Для внесения изменений в реестр

// - - - - - Работа с ядром приложения ###

extern void embed_main_exe_core_data(void);
extern bool is_single_instance(void);
extern QString getFileSHA256(QFile & hashing_file);

// - - - - - Работа с переменными среды ###

struct AppEnviroment {
    QString temp_dir;
    QString working_dir;
    QString main_exe;
    QString recover_exe;
    QString updater_exe;
    QString logfile;
    QString config;
    QString appname;
    QString version;
    QString pid;
    QString new_version;
};

//Получаем данные для переменные среды
//(для редактирования или сразу передаче следующей функции)
extern AppEnviroment get_env_data(void);
//Устанавливаем перед созданием процесса
extern void set_env(const AppEnviroment & app_env);
//Получаем переменные среды (в начале нового процесса)
extern AppEnviroment read_env(void);
//Логирование переменных среды
extern void write_env(void);

} //namespace appfuncs
} //namespace commn


#endif // APPFUNCS_H
