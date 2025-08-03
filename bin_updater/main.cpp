// updater/main.cpp
// Собирается в отдельный бинарник

#include <QCoreApplication>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QString>
#include <string>
#include <map>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <windows.h>

using namespace std;

static void wait_for_process_exit(qint64 pid);

//Функция для сбора информации об обновлении
static void get_update_data(const QString & temp_dir, const QString & work_dir,
                            std::map<std::filesystem::path, std::filesystem::path> & newfile_mapper,
                            std::map<std::filesystem::path, std::filesystem::path> & replace_mapper,
                            std::vector<std::filesystem::path> & delfile_vector,
                            std::vector<std::filesystem::path> & deldir_vector);

//Фунции добавления, замены либо удаления файлов/каталогов
static void addnew_files(const std::map<std::filesystem::path, std::filesystem::path> & newfile_mapper);
static void replace_files(const std::map<std::filesystem::path, std::filesystem::path> & replace_mapper);
static void delete_files(const std::vector<std::filesystem::path> & files);
static void delete_dirs(const std::vector<std::filesystem::path> & dirs);
static void parse_string(const std::string & str, std::string & tag, std::string & value);

int main() {
    //Содержит все новые файлы, которые подлежат замене
    //А также файлы .txt которые содержат внутри тэг, DELFILE/DELDIR:%path%,
    //которые отмечают то, какие файлы/каталоги подлежат удалению
    QString temp_dir  = qgetenv("TEMP_DIR");
    //Каталог, который содержит в корне main.exe и все необходимые ресурсы
    QString work_dir  = qgetenv("WORKING_DIR");
    //Имя из оригинального exe чтобы не было лишних ошибок
    QString application_name = qgetenv("APP_NAME");
    //Бинарник основного приложения
    QString main_exe  = qgetenv("MAIN_EXE");
    //Бинарник, который выполняет отмену изменений в случае неудачи (какие-то серьезные ошибки)
    QString recover_exe  = qgetenv("RECOVER_EXE");
    //Новая версия, которая должна быть помещена в реестр после завершения обновлений
    QString version = qgetenv("VERSION");
    //PID основного процесса, из которого был запущен текущий
    QString parent_pid_str = qgetenv("PARENT_PID");

    // Проверка существования путей
    if (!QDir(temp_dir).exists() || !QDir(work_dir).exists()) {
        qDebug() << "Заданы неверные пути временной и рабочей директорий";
        return 1;
    }

    // Ждём завершения родителя
    bool ok = false;
    qint64 parent_pid = parent_pid_str.toLongLong(&ok);
    if (ok) {
        qDebug() << "Ожидание завершения родительского процесса...";
        wait_for_process_exit(parent_pid);
    }

    //Для хранения путей новых файлов (пара temp_file_path, target_file_path)
    std::map<std::filesystem::path, std::filesystem::path> newfile_mapper;
    //Для хранения путей файлов, подлежащих замене (пара actual_version_path, old_version_path)
    std::map<std::filesystem::path, std::filesystem::path> raplace_mapper;
    //Для хранения путей файлов, подлежащих удалению
    std::vector<std::filesystem::path> delfile_vector;
    // Для хранения путей каталогов, подлежищих удалению
    std::vector<std::filesystem::path> deldir_vector;

    try {
        //Далее - запускаем рекурсивную фунцию get_update_data для сбора данных об элементах, проходящая в temp_dir
        //Функция должна заполнить структуры данных raplace_mapper delfile_vector deldir_vector
        //При этом открывая каждый .txt файл c проверкой наличия записи внутри "DELFILE:%path%"/"DELDIR:%path%"
        //Все встречные файлы отмечаются как новые, и для них ищется аналогичный файл в work_dir
        get_update_data(temp_dir, work_dir, newfile_mapper, raplace_mapper, delfile_vector, deldir_vector);
        qDebug() << "Получены данные для обновлений";
        //Запускаем функцию копирования новых файлов
        addnew_files(newfile_mapper);
        qDebug() << "Произведено добавление новых файлов";
        //Запускаем функцию замены старых файлов на новые raplace_mapper
        replace_files(raplace_mapper);
        qDebug() << "Произведена замена файлов";
        //Запускаем функцию удаления старых версий (помечены при замене) delfile_vector
        delete_files(delfile_vector);
        qDebug() << "Выполнено удаление файлов";
        //Запускаем функцию удаления элементов, подлежащих удалению deldir_vector
        delete_dirs(deldir_vector);
        qDebug() << "Выполнено удаление каталогов";
    }
    catch (std::runtime_error & ex)
    {
        qDebug() << "Ошибка при замене/удалении файлов:" << ex.what();
        //Тут по идее запуск recover.exe
    }
    catch (std::exception & ex)
    {
        qDebug() << "Ошибка при замене/удалении файлов:" << ex.what();
        //Тут по идее запуск recover.exe
    }

    //Удаляем каталог temp_dir
    try {
        std::filesystem::remove_all(std::filesystem::path(temp_dir.toStdWString()));
    }
    catch (const std::exception& e)
    {
        qDebug() << "Ошибка при удалении временной директории:" << e.what();
    }

    //Заменяем ключ реестра "HKEY_CURRENT_USER\\Software\\" + application_name
    QSettings settings("HKEY_CURRENT_USER\\Software\\" + application_name, QSettings::NativeFormat);
    settings.setValue("Version", version);

    // Успешное выполнение
    qDebug() << "Обновление завершено!";

    //Перезапуск основного бинарника main.exe
    if (!QProcess::startDetached(main_exe)) {
        qDebug() << "Ошибка при перезапуске основного приложения.";
        return 1;
    }

    return 0;
}

// Ждёт завершения процесса по PID
static void wait_for_process_exit(qint64 pid)
{
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (hProcess != nullptr) {
        WaitForSingleObject(hProcess, INFINITE);
        CloseHandle(hProcess);
    } else {
        qDebug() << "Не удалось открыть процесс с PID" << pid << ", возможно он уже завершён.";
    }
}

// Рекурсивно обходит temp_dir и заполняет структуры
// Если это файл .txt то он может содержать тэги, которые указывают на удаляемый файл/каталог
// Все остальные файлы сразу же добавляются в map для замены файлов
static void get_update_data(const QString& temp_dir, const QString& work_dir,
                            std::map<std::filesystem::path, std::filesystem::path>& newfile_mapper,
                            std::map<std::filesystem::path, std::filesystem::path>& replace_mapper,
                            std::vector<std::filesystem::path>& delfile_vector,
                            std::vector<std::filesystem::path>& deldir_vector)
{
    namespace fs = std::filesystem;
    //Устанавливаем временную и рабочую директории
    const fs::path temp_path = fs::path(temp_dir.toStdWString());
    const fs::path work_path = fs::path(work_dir.toStdWString());

    for (const auto& entry : fs::recursive_directory_iterator(temp_path))
    {
        const fs::path& temp_fullpath = entry.path();
        const fs::path& relpath = fs::relative(temp_path, temp_fullpath);
        const fs::path& work_fullpath = work_path / relpath;

        if(!fs::exists(work_fullpath))
        {
            newfile_mapper[temp_fullpath] = work_fullpath;
            continue;
        }

        if (fs::is_regular_file(temp_fullpath))
        {
            //Проверим на наличие тэгов каждый .txt, что укажет что это delfile и добавляется в вектор
            if (temp_fullpath.extension() == L".txt")
            {
                std::ifstream file(temp_fullpath);
                std::string line;
                if (file && std::getline(file, line)) //Если найдем в нем тэг, то это notice-file
                {
                    std::string tag, value;
                    parse_string(line, tag, value);

                    if (tag == "DELFILE")
                    {
                        delfile_vector.emplace_back(work_path / value);
                    }
                    else if (tag == "DELDIR")
                    {
                        deldir_vector.emplace_back(work_path / value);
                    }
                    continue;
                }
            }

            //Все остальные файлы добавляем сразу
            replace_mapper[temp_fullpath] = work_fullpath;
        }
    }
}

// Добавляем новые файлы
static void addnew_files(const std::map<std::filesystem::path, std::filesystem::path>& newfile_mapper)
{
    for (const auto& [temp_fullpath, work_fullpath] : newfile_mapper)
    {
        std::filesystem::copy_file(temp_fullpath, work_fullpath);
        //Здесь по идее записи в журнал для recover.exe
    }
}

// Заменяет старые версии файлов на новые
static void replace_files(const std::map<std::filesystem::path, std::filesystem::path>& replace_mapper)
{
    for (const auto& [temp_fullpath, work_fullpath] : replace_mapper)
    {
        std::filesystem::copy_file(temp_fullpath, work_fullpath, std::filesystem::copy_options::overwrite_existing);
        //Здесь по идее записи в журнал для recover.exe
    }
}

// Удаляет файлы из списка
static void delete_files(const std::vector<std::filesystem::path>& files)
{
    for (const auto& file_path : files)
    {
        std::filesystem::remove(file_path);
        //Здесь по идее записи в журнал для recover.exe
    }
}

// Удаляет каталоги из списка
static void delete_dirs(const std::vector<std::filesystem::path>& dirs) {
    for (const auto& dir_path : dirs)
    {
        std::filesystem::remove_all(dir_path);
        //Здесь по идее записи в журнал для recover.exe
    }
}

void parse_string(const std::string& str, std::string& tag, std::string& value)
{
    size_t pos = str.find(':');
    if (pos != std::string::npos)
    {
        tag = str.substr(0, pos);
        value = str.substr(pos + 1);
    }
    else
    {
        tag.clear();
        value.clear();
    }
}

