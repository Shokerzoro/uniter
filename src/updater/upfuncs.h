#ifndef UPFUNCS_H
#define UPFUNCS_H

#include <filesystem>
#include <vector>
#include <map>
#include <QString>

#include "typesheader.h"

namespace updater {
namespace upfuncs {

//Find/define a tag inside a .txt file, return true if it's a notice file
extern bool is_notice(const std::filesystem::path &file, QString & tag, QString & value);

//Waiting for parent process to complete
extern void wait_for_process_exit(QString & parent_pid) noexcept;

//Function for collecting update information
extern void get_update_data(QString & temp_dir,
                            QString & work_dir,
                            PathMapper & newdir_mapper,
                            PathMapper & newfile_mapper,
                            PathMapper & replace_mapper,
                            PathVector & delfile_vector,
                            PathVector & deldir_vector);

//Functions for adding files/directories, replacing or deleting files/directories
extern void addnew_dirs(PathMapper & newdir_mapper);
extern void addnew_files(PathMapper & newfile_mapper);
extern void replace_files(PathMapper & replace_mapper);
extern void delete_files(PathVector & delfile_vector);
extern void delete_dirs(PathVector & deldir_vector);

} //upfuncs
} //updater

#endif // UPFUNCS_H
