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

}



#endif // APPFUNCS_H
