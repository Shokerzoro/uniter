
#include <fstream>
#include <filesystem>
#include <vector>
#include <map>
#include <QCoreApplication>
#include <QDebug>
#include <windows.h>

#include "typesheader.h"

namespace updater {
namespace upfuncs {

// Adding new directories
void addnew_dirs(std::map<std::filesystem::path, std::filesystem::path> & newdir_mapper)
{
    for (const auto& [temp_fullpath, work_fullpath] : newdir_mapper)
    {
        qDebug() << "addnew_dir: copy directory " << QString::fromStdWString(temp_fullpath.wstring())
        << " to " << QString::fromStdWString(work_fullpath.wstring());

        std::filesystem::copy(temp_fullpath, work_fullpath,
                              std::filesystem::copy_options::recursive);

        //Here's the idea of ​​logging for recover.exe
    }
}

// Adding new files
void addnew_files(std::map<std::filesystem::path, std::filesystem::path> & newfile_mapper)
{
    for (const auto& [temp_fullpath, work_fullpath] : newfile_mapper)
    {
        qDebug() << "addnew_files: " << temp_fullpath.string();
        std::filesystem::copy_file(temp_fullpath, work_fullpath);
        //Here's the idea of ​​logging for recover.exe
    }
}

// Replaces old versions of files with new ones
void replace_files(std::map<std::filesystem::path, std::filesystem::path> & replace_mapper)
{
    for (const auto& [temp_fullpath, work_fullpath] : replace_mapper)
    {
        qDebug() << "replace_files: " << QString::fromStdString(temp_fullpath.string());

        // If the destination file exists, delete it
        std::error_code ec;
        if (std::filesystem::exists(work_fullpath, ec))
        {
            if (!std::filesystem::remove(work_fullpath, ec) || ec)
            {
                qWarning() << "replace_files: failed to remove existing file:" << QString::fromStdString(work_fullpath.string()) << ", error:" << ec.message().c_str();
                continue; // or break, or throw an exception - decide for yourself
            }
        }

        // Copying the file
        if (!std::filesystem::copy_file(temp_fullpath, work_fullpath, std::filesystem::copy_options::none, ec) || ec)
        {
            qWarning() << "replace_files: failed to copy file from" << QString::fromStdString(temp_fullpath.string())
            << "to" << QString::fromStdString(work_fullpath.string()) << ", error:" << ec.message().c_str();
            continue;
        }

        // TODO: Add log entry for recover.exe
    }
}

// Removes files from the list
void delete_files(std::vector<std::filesystem::path> & files)
{
    for (const auto& file_path : files)
    {
        qDebug() << "delete_file: " << file_path.string();
        std::filesystem::remove(file_path);
        //Here's the idea of ​​logging for recover.exe
    }
}

// Removes directories from the list
void delete_dirs(std::vector<std::filesystem::path> & dirs) {
    for (const auto& dir_path : dirs)
    {
        qDebug() << "delete_dir: " << dir_path.string();
        std::filesystem::remove_all(dir_path);
        //Here's the idea of ​​logging for recover.exe
    }
}

//We look inside the file for a line with a tag and path
bool is_notice(const std::filesystem::path & file, QString & tag, QString & value)
{
    //If there is a .txt extension
    if (file.extension() == ".txt")
    {
        std::ifstream in(file);
        if (!in.is_open())
            throw std::runtime_error("is_notice: can't open " + file.string());

        std::string line;
        std::getline(in, line);
        in.close();

        auto pos = line.find(':');
        if (pos != std::string::npos) {
            tag = QString::fromStdString(line.substr(0, pos));
            value = QString::fromStdString(line.substr(pos + 1));
            return tag == "DELFILE" || tag == "DELDIR";
        }
    }
    return false;
}

// Waits for process completion based on PID
void wait_for_process_exit(QString & parent_pid_str) noexcept
{
    bool ok = false;
    qint64 parent_pid = parent_pid_str.toLongLong(&ok);
    if (ok) {
        qDebug() << "Updater: waiting parent process exit";

        HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(parent_pid));
        if (hProcess != nullptr) {
            WaitForSingleObject(hProcess, INFINITE);
            CloseHandle(hProcess);
        } else {
            qDebug() << "Updater: cant open process " << parent_pid << ", it has exited maybe";
        }
    }
}

// Recursively traverses temp_dir and fills structures
// If this is a .txt file then it may contain tags that indicate the file/directory to be deleted
// All other files are immediately added to the map to replace the files
void get_update_data(QString & qtemp_dir,
                     QString & qwork_dir,
                     std::map<std::filesystem::path, std::filesystem::path> & newdir_mapper,
                     std::map<std::filesystem::path, std::filesystem::path> & newfile_mapper,
                     std::map<std::filesystem::path, std::filesystem::path> & replace_mapper,
                     std::vector<std::filesystem::path> & delfile_vector,
                     std::vector<std::filesystem::path> & deldir_vector)
{
    std::filesystem::path temp_dir = std::filesystem::path(qtemp_dir.toStdWString());
    std::filesystem::path work_dir = std::filesystem::path(qwork_dir.toStdWString());
    //We declare a tag and value for checking tags DELFILE DELDIR
    QString tag, value;

    // We get the absolute path to the current executable file (updater.exe)
    const Path updater_path = std::filesystem::canonical(QCoreApplication::applicationFilePath().toStdWString());

    for (auto rec_iter = std::filesystem::recursive_directory_iterator(temp_dir);
         rec_iter != std::filesystem::recursive_directory_iterator(); ++rec_iter)
    {
        const std::filesystem::directory_entry & entry = *rec_iter;
        const Path full_temp_path = entry.path();
        const Path rel_path = std::filesystem::relative(full_temp_path, temp_dir);
        const Path full_work_path = work_dir / rel_path;

        //If a regular file and is_notice(full_temp_path, tag, value) returns true
        if(entry.is_regular_file() && is_notice(full_temp_path, tag, value)) {
            const Path target_path = work_dir / value.toStdWString();

            // We exclude the removal of updater.exe
            if (std::filesystem::equivalent(target_path, updater_path)) {
                qDebug() << "get_update_data: skipping deletion of updater.exe";
                continue;
            }

            //Depending on the tag, add it to delfile_vector or deldir_vector
            if(tag == "DELFILE")
            {
                qDebug() << "get_update_data: delfile found: " << value;
                delfile_vector.emplace_back(target_path);
                continue;
            }
            else if (tag == "DELDIR")
            {
                qDebug() << "get_update_data: deldir found: " << value;
                deldir_vector.emplace_back(target_path);
                continue;
            }
        }

        //If a regular file and the same one exists in work_dir
        if(entry.is_regular_file() && std::filesystem::exists(full_work_path))
        {
            // We exclude the replacement of updater.exe
            if (std::filesystem::equivalent(full_work_path, updater_path))
            {
                qDebug() << "get_update_data: skipping replacement of updater.exe";
                continue;
            }

            //Add to replace_mapper
            qDebug() << "get_update_data: remplacement found: " << rel_path.string();
            replace_mapper[full_temp_path] = full_work_path;
            continue;
        }

        //If a regular file does not exist in work_dir
        if(entry.is_regular_file())
        {
            //Add to newfile_mapper
            qDebug() << "get_update_data: new file found: " << rel_path.string();
            newfile_mapper[full_temp_path] = full_work_path;
            continue;
        }

        //If the directory and the same does not exist in work_dir
        if(entry.is_directory() && !std::filesystem::exists(full_work_path))
        {
            //Add to newdir_mapper
            qDebug() << "get_update_data: new dir found: " << rel_path.string();
            newdir_mapper[full_temp_path] = full_work_path;
            rec_iter.disable_recursion_pending();
            continue;
        }
    }
}

} //upfuncs
} //updater
