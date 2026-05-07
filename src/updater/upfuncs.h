#ifndef UPFUNCS_H
#define UPFUNCS_H

#include <filesystem>
#include <vector>
#include <map>
#include <QString>

#include "typesheader.h"

namespace updater {
namespace upfuncs {

//Найти/определить тэг внутри .txt файла, возрат true - если это notice file
extern bool is_notice(const std::filesystem::path &file, QString & tag, QString & value);

//Ожидание завершения родительского процесса
extern void wait_for_process_exit(QString & parent_pid) noexcept;

//Функция для сбора информации об обновлении
extern void get_update_data(QString & temp_dir,
                            QString & work_dir,
                            PathMapper & newdir_mapper,
                            PathMapper & newfile_mapper,
                            PathMapper & replace_mapper,
                            PathVector & delfile_vector,
                            PathVector & deldir_vector);

//Фунции добавления файлов/каталогов, замены либо удаления файлов/каталогов
extern void addnew_dirs(PathMapper & newdir_mapper);
extern void addnew_files(PathMapper & newfile_mapper);
extern void replace_files(PathMapper & replace_mapper);
extern void delete_files(PathVector & delfile_vector);
extern void delete_dirs(PathVector & deldir_vector);

} //upfuncs
} //updater

#endif // UPFUNCS_H
