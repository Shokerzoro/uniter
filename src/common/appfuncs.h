#ifndef APPFUNCS_H
#define APPFUNCS_H

#include <QProcessEnvironment>
#include <QString>
#include <QFile>
#include <filesystem>

namespace common {
namespace appfuncs {

using Path = std::filesystem::path;

// ---Logging ---

//Opens log_stream
extern void open_log(const QString & logfilepath);
//Writes to log_stream
extern void write_log(const QString & msg);
//Writes time and date to log_stream
extern void log_time(void);
//Close the log
extern void close_log(void);

// --- Working with the registry ---

extern void add_reg_key(void); // Add an entry, just to test the rest
extern QString get_reg_version(void); // Get current version
extern void set_reg_new_version(void); // To make changes to the registry

// - - - - - Working with the application core ###

extern void embed_main_exe_core_data(void);
extern bool is_single_instance(void);
extern QString getFileSHA256(QFile & hashing_file);

// - - - - - Working with environment variables ###

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

//Getting data for environment variables
//(for editing or immediately passing to the next function)
extern AppEnviroment get_env_data(void);
//Install before creating a process
extern void set_env(const AppEnviroment & app_env);
//Getting environment variables (at the start of a new process)
extern AppEnviroment read_env(void);
//Logging environment variables
extern void write_env(void);

} //namespace appfuncs
} //namespace commn


#endif // APPFUNCS_H
