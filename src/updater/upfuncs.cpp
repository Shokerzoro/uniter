
#include <fstream>
#include <filesystem>
#include <vector>
#include <map>
#include <QCoreApplication>
#include <QDebug>
#include <windows.h>

using Path = std::filesystem::path;
using PathMapper = std::map<std::filesystem::path, std::filesystem::path>;
using PathVector = std::vector<std::filesystem::path>;

namespace upfuncs {

// Добавляем новые каталоги
void addnew_dirs(PathMapper & newdir_mapper)
{
    for (const auto& [temp_fullpath, work_fullpath] : newdir_mapper)
    {
        qDebug() << "addnew_dir: copy directory " << QString::fromStdWString(temp_fullpath.wstring())
        << " to " << QString::fromStdWString(work_fullpath.wstring());

        std::filesystem::copy(temp_fullpath, work_fullpath,
                              std::filesystem::copy_options::recursive);

        //Здесь по идее записи в журнал для recover.exe
    }
}

// Добавляем новые файлы
void addnew_files(PathMapper & newfile_mapper)
{
    for (const auto& [temp_fullpath, work_fullpath] : newfile_mapper)
    {
        qDebug() << "addnew_files: " << temp_fullpath.string();
        std::filesystem::copy_file(temp_fullpath, work_fullpath);
        //Здесь по идее записи в журнал для recover.exe
    }
}

// Заменяет старые версии файлов на новые
void replace_files(PathMapper & replace_mapper)
{
    for (const auto& [temp_fullpath, work_fullpath] : replace_mapper)
    {
        qDebug() << "replace_files: " << QString::fromStdString(temp_fullpath.string());

        // Если файл назначения существует — удаляем
        std::error_code ec;
        if (std::filesystem::exists(work_fullpath, ec))
        {
            if (!std::filesystem::remove(work_fullpath, ec) || ec)
            {
                qWarning() << "replace_files: failed to remove existing file:" << QString::fromStdString(work_fullpath.string()) << ", error:" << ec.message().c_str();
                continue; // или break, или бросить исключение — решай сам
            }
        }

        // Копируем файл
        if (!std::filesystem::copy_file(temp_fullpath, work_fullpath, std::filesystem::copy_options::none, ec) || ec)
        {
            qWarning() << "replace_files: failed to copy file from" << QString::fromStdString(temp_fullpath.string())
            << "to" << QString::fromStdString(work_fullpath.string()) << ", error:" << ec.message().c_str();
            continue;
        }

        // TODO: Добавить запись в журнал для recover.exe
    }
}

// Удаляет файлы из списка
void delete_files(PathVector & files)
{
    for (const auto& file_path : files)
    {
        qDebug() << "delete_file: " << file_path.string();
        std::filesystem::remove(file_path);
        //Здесь по идее записи в журнал для recover.exe
    }
}

// Удаляет каталоги из списка
void delete_dirs(PathVector & dirs) {
    for (const auto& dir_path : dirs)
    {
        qDebug() << "delete_dir: " << dir_path.string();
        std::filesystem::remove_all(dir_path);
        //Здесь по идее записи в журнал для recover.exe
    }
}

//Ищем внутри файла строку с тэгом и путем
bool is_notice(const std::filesystem::path & file, QString & tag, QString & value)
{
    //Если есть расширение .txt
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

// Ждёт завершения процесса по PID
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

// Рекурсивно обходит temp_dir и заполняет структуры
// Если это файл .txt то он может содержать тэги, которые указывают на удаляемый файл/каталог
// Все остальные файлы сразу же добавляются в map для замены файлов
void get_update_data(QString & qtemp_dir,
                     QString & qwork_dir,
                     PathMapper & newdir_mapper,
                     PathMapper & newfile_mapper,
                     PathMapper & replace_mapper,
                     PathVector & delfile_vector,
                     PathVector & deldir_vector)
{
    std::filesystem::path temp_dir = std::filesystem::path(qtemp_dir.toStdWString());
    std::filesystem::path work_dir = std::filesystem::path(qwork_dir.toStdWString());
    //Объявляем тэг и значение для проверки тэгов DELFILE DELDIR
    QString tag, value;

    // Получаем абсолютный путь до текущего исполняемого файла (updater.exe)
    const Path updater_path = std::filesystem::canonical(QCoreApplication::applicationFilePath().toStdWString());

    for (auto rec_iter = std::filesystem::recursive_directory_iterator(temp_dir);
         rec_iter != std::filesystem::recursive_directory_iterator(); ++rec_iter)
    {
        const std::filesystem::directory_entry & entry = *rec_iter;
        const Path full_temp_path = entry.path();
        const Path rel_path = std::filesystem::relative(full_temp_path, temp_dir);
        const Path full_work_path = work_dir / rel_path;

        //Если обычный файл и is_notice(full_temp_path, tag, value) возвращает true
        if(entry.is_regular_file() && is_notice(full_temp_path, tag, value)) {
            const Path target_path = work_dir / value.toStdWString();

            // Исключаем удаление updater.exe
            if (std::filesystem::equivalent(target_path, updater_path)) {
                qDebug() << "get_update_data: skipping deletion of updater.exe";
                continue;
            }

            //В зависимости от тэга добваляем в delfile_vector или deldir_vector
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

        //Если обычный файл и такой же существует в work_dir
        if(entry.is_regular_file() && std::filesystem::exists(full_work_path))
        {
            // Исключаем замену updater.exe
            if (std::filesystem::equivalent(full_work_path, updater_path))
            {
                qDebug() << "get_update_data: skipping replacement of updater.exe";
                continue;
            }

            //Добавляем в replace_mapper
            qDebug() << "get_update_data: remplacement found: " << rel_path.string();
            replace_mapper[full_temp_path] = full_work_path;
            continue;
        }

        //Если обычный файл и такого не существует в work_dir
        if(entry.is_regular_file())
        {
            //Добавляем в newfile_mapper
            qDebug() << "get_update_data: new file found: " << rel_path.string();
            newfile_mapper[full_temp_path] = full_work_path;
            continue;
        }

        //Если каталог и такого же не существует в work_dir
        if(entry.is_directory() && !std::filesystem::exists(full_work_path))
        {
            //Добавляем в newdir_mapper
            qDebug() << "get_update_data: new dir found: " << rel_path.string();
            newdir_mapper[full_temp_path] = full_work_path;
            rec_iter.disable_recursion_pending();
            continue;
        }
    }
}

}
