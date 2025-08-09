#ifndef APPFUNCS_H
#define APPFUNCS_H

#include <QProcessEnvironment>
#include <QString>
#include <filesystem>

namespace appfuncs {

using Path = std::filesystem::path;

extern void embed_meta(void);
extern QProcessEnvironment set_env(QString & version) noexcept;
extern void get_env(Path & temp_dir, Path & working_dir, QString & app_name, QString & parent_pid,
                    QString & main_exe, QString & recover_exe, QString & new_vers) noexcept;

/*

extern void update_reg_version(QString & new_version); //Для внесения изменений в реестр
extern void set_log_file(Path & logfilepath); //для создания логов
*/

}



#endif // VERSION_H
