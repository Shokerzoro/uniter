#ifndef APPFUNCS_H
#define APPFUNCS_H

#include <QProcessEnvironment>
#include <QString>
#include <QFile>
#include <filesystem>

namespace appfuncs {

using Path = std::filesystem::path;

extern void embed_meta(void);
extern QProcessEnvironment set_env(QString & version) noexcept;
extern void get_env(Path & temp_dir, Path & working_dir, QString & app_name, QString & parent_pid,
                    QString & main_exe, QString & recover_exe, QString & new_vers) noexcept;

extern QString getFileSHA256(QFile & hashing_file);

}



#endif // VERSION_H
