#include "UpdaterWorker.h"
#include "../common/netfunc.h"
#include "../common/excepts.h"
#include "../common/appfuncs.h"
#include "../common/unetmestags.h"

#include <QApplication>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QString>
#include <QDebug>
#include <QTimer>

#include <vector>
#include <exception>

//Базовая инициализация, выполнение соединений
UpdaterWorker::UpdaterWorker(QObject *parent) : QObject{parent}
{
    rem_temp_dir_data = false;
    ServerData = server_data::NONE;
    NetState = net_state::OFFLINE;
    ProtocolState = protocol_state::UNKNOWN;
    port = -1;

    //Сразу коннектим слоты и сигналы для событийного паттерна
    connect(this, &UpdaterWorker::signalFindConfig, this, &UpdaterWorker::FindConfig, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalGetServerData, this, &UpdaterWorker::GetServerData, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalMakeConnect, this, &UpdaterWorker::MakeConnect, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalMoreSocketData, this, &UpdaterWorker::GotSockData, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalNetError, this, &UpdaterWorker::OnNetError, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalRefreshServerData, this, &UpdaterWorker::RefreshServerData, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalRefuseUpdates, this, &UpdaterWorker::RefuseUpdates, Qt::QueuedConnection);

    //Динамически выделяем сокет
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &UpdaterWorker::OnConnected, Qt::QueuedConnection);
    connect(socket, &QTcpSocket::readyRead, this, &UpdaterWorker::GotSockData, Qt::QueuedConnection);
    connect(socket, &QTcpSocket::disconnected, this, &UpdaterWorker::Disconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &UpdaterWorker::OnNetError);

    basePath = QCoreApplication::applicationDirPath();
    version = QCoreApplication::applicationVersion();
    qDebug() << "UpdaterWorker: обнаружена версия:" << version;
    user_temp_dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    qDebug() << "UpdaterWorker: директория пользовательских данных установлена:" << user_temp_dir.path();
    config_file = basePath.filePath("config/config.ini");

    updater_exe = basePath.filePath("bin/updater.exe");
    recover_exe = basePath.filePath("bin/recover.exe");
}

//Деструктор
UpdaterWorker::~UpdaterWorker(void)
{
    qDebug() << "UpdaterWorker: вызов деструктора!";

    if(socket->state() == QAbstractSocket::ConnectedState)
        socket->abort();
}

void UpdaterWorker::set_rem_temp_dir_data(bool option)
{
    rem_temp_dir_data = option;
    qDebug() << "UpdaterWorker: rem_temp_dir_data = " << option;
}

//Основная функция, заканчивающаяся в случае успеха подключением
void UpdaterWorker::StartRun(void)
{
    qDebug() << "UpdaterWorker начал выполнение!";

    if (!QFile::exists(updater_exe))
    {
        qDebug() << "UpdaterWorker: не обнаружен updater.exe!";
        emit signalNoUpdaterExe();
    }
    if (!QFile::exists(recover_exe))
    {
        qDebug() << "UpdaterWorker: не обнаружен recover_exe!";
        emit signalNoRecoverExe();
    }

    //Отправляем сигналы виджетам
    emit signalSetVersion(version);
    emit signalOffline();
    //Запускаем процесс поиска данных
    emit signalFindConfig();
}

//Метод для поиска конфигурационного файла
void UpdaterWorker::FindConfig(void)
{
    if (!QFile::exists(config_file))
    {
        qDebug() << "INI-файл не найден!";
        emit signalNoServerData(); //Неудача
        //Установка таймера на refresh
        QTimer::singleShot(UpdateTimeouts::findConfigRetryMs, this, &UpdaterWorker::FindConfig);
    }
    else
    {
        qDebug() << "INI-файл найден!";
        ServerData = server_data::FILEEXISTS;
        emit signalGetServerData();
    }
}

//Метод для получения данных о сервере
void UpdaterWorker::GetServerData(void)
{
    if(ServerData == server_data::FILEEXISTS)
    {
        QSettings settings(config_file, QSettings::IniFormat);
        settings.beginGroup("update_server");

        this->IPv4_address = settings.value("address").toString();
        this->port = settings.value("port").toInt();
        settings.endGroup();

        if((!IPv4_address.isEmpty()) && (port != -1))
        {
            ServerData = server_data::FOUND;
            qDebug() << "Найдены данные сервера";
            emit signalMakeConnect();
        }
        else //неудача
        {
            emit signalNoServerData();
            //Установка таймера на refresh, чтобы заново начать поиск конфига
            QTimer::singleShot(UpdateTimeouts::getServerDataRetryMs, this, &UpdaterWorker::RefreshServerData);
        }
    }
}

//Полное "перечтение" конфигурационного файла
void UpdaterWorker::RefreshServerData(void)
{
    socket->abort();
    ServerData = server_data::NONE;
    NetState = net_state::OFFLINE;
    ProtocolState = protocol_state::UNKNOWN;
    emit signalOffline();
    //Запуск цикла по сути с начальной точки
    emit signalMakeConnect();
}

//Метод установки соединения с сервером
void UpdaterWorker::MakeConnect()
{
    if(ServerData == server_data::NONE)
    {
        qDebug() << "UpdaterWorker не владеет доступом к файлу конфигурации!";
        emit signalFindConfig();
        return;
    }
    if(ServerData == server_data::FILEEXISTS)
    {
        qDebug() << "UpdaterWorker не владеет данными с сервера!";
        emit signalGetServerData();
        return;
    }
    if(ServerData == server_data::FOUND)
    {
        if (socket->state() == QAbstractSocket::UnconnectedState)
        {
            qDebug() << "UpdaterWorker: попытка подключения к серверу...";
            socket->connectToHost(IPv4_address, port);
            return;
        }
        else
        {
            qDebug() << "Сокет уже подключён или подключается";
        }
    }
}

//При соединении выполняет отправку идентификационного хэдера
void UpdaterWorker::OnConnected(void)
{
    qDebug() << "UpdaterWorker: соединен с сервером обновлений!";
    NetState = net_state::ONLINE;
    ProtocolState = protocol_state::UNKNOWN;
    ComType = communication_type::HEADERS;
    emit signalOnline();

    //!!Отправка заголовка "UNET-MES" для инициализации обмена заголовками
    netfuncs::build_header(header, UMTags::PROTOCOL, UMTags::UNETMES);
    netfuncs::send_header(header, buffer, socket);
}

//Основной слот, который проходит по состояниям
void UpdaterWorker::GotSockData(void)
{
    qDebug() << "UpdaterWorker: got data on socket!";
        try { //Try
        switch (ComType)
        {
        case communication_type::HEADERS:
        {
            this->HandleHeadersExchange();
            break;
        }
        case communication_type::BINARY:
        {
            this->HandleDownFile();
            break;
        }
        default:
        {
            throw std::logic_error("UpdaterWorker: GotSockData: unpropriate updating state");
            break;
        }
        }
        } //Try
        catch (excepts::unacceptable & un)
        {
            qDebug() << "UpdaterWorker: недопустимое исключние!";
            qDebug() << un.what();
            //Остановка воркера
            emit signalOffline();
            this->deleteLater();
        }
        catch (std::runtime_error & ex)
        {
            qDebug() << "UpdaterWorker: ошибка времени выполненя!";
            qDebug() << ex.what();
            //Перезапуск с обновлением данных
            emit signalRefreshServerData();

            return;
        }
        catch(std::invalid_argument & ex)
        {
            qDebug() << "UpdaterWorker: ошибка протокола или неверные данные!";
            qDebug() << ex.what();
            //Перезапуск сервиса
            emit signalNetError();
            return;
        }
        catch(std::exception & ex)
        {
            qDebug() << "UpdaterWorker: неизвестная ошибка!";
            qDebug() << ex.what();
            emit signalRefreshServerData();
            return;
        }

}

//Будет срабатывать только при работе сокета, т.к. в промежутках сокет закрыт
//Поэтому не перекроет таймер в RefuseUpdates
void UpdaterWorker::OnNetError(void)
{
    if(ProtocolState != protocol_state::COMPLETE)
    {
        qDebug() << "UpdaterWorker: ошибка TCP соединения!";
        qDebug() << "Error string:" << socket->errorString();

        if(socket->state() == QAbstractSocket::ConnectedState)
            socket->abort();
        if(filestream.is_open())
            filestream.close();
        NetState = net_state::OFFLINE;
        ProtocolState = protocol_state::UNKNOWN;
        ComType = communication_type::NONE;
        emit signalOffline();
        //Постановка небольшого таймера на реконнект
        QTimer::singleShot(UpdateTimeouts::netReconnectMs, this, &UpdaterWorker::MakeConnect);
    }
}

void UpdaterWorker::Disconnected(void)
{
    if(ProtocolState != protocol_state::COMPLETE)
    {
        qDebug() << "UpdaterWorker: обрыв соединения с сервером!";
        if(socket->state() == QAbstractSocket::ConnectedState)
            socket->abort();
        if(filestream.is_open())
            filestream.close();
        NetState = net_state::OFFLINE;
        ProtocolState = protocol_state::UNKNOWN;
        ComType = communication_type::NONE;
        emit signalOffline();
        //Постановка небольшого таймера на реконнект
        QTimer::singleShot(UpdateTimeouts::netReconnectMs, this, &UpdaterWorker::MakeConnect);
    }
}

// - - - - - - - - Методы для сетевого взаимодейтсвия - - - -

void UpdaterWorker::HandleHeadersExchange(void)
{
    qDebug() << "UpdaterWorker: HandleHeadersExchange: proccesing!";

    if(ProtocolState != protocol_state::COMPLETE && socket->bytesAvailable() != 0)
    {
        //Всегда читаем заголовок
        netfuncs::read_header(header, buffer, socket);
        netfuncs::parse_header(header, tag, value);

        switch (ProtocolState)
        {
        //Обмен типом протокола (уже отправили "PROTOCOL:UNET-MES" со своей стороны)
        //Устанавливаем PROPER состояние протокола и отправляем "VERSION:%version%"
        case protocol_state::UNKNOWN:
        {
            if(tag == UMTags::PROTOCOL && value == UMTags::UNETMES)
            {
                qDebug() << "UpdaterWorker: HandleHeadersExchange: proccesing!";
                this->HandleUnknown();
            }
            else
                throw std::invalid_argument("UpdaterWorker: ошибка идентификации протокола...");
            break;
        }
        //Что означает, что мы ждем получание новой версии
        //Устанавлиеваем состояние UPDATING - мы получаем название новой версии
        case protocol_state::PROPER:
        {
            if(tag == UMTags::PROTOCOL)
            {
                qDebug() << "UpdaterWorker: HandleHeadersExchange: proccesing!";
                this->HandleProper();
            }
            else
                throw std::invalid_argument("UpdateWorker: неправильный ответ на запрос о наличии обновления!");
            break;
        }
        //Получаем информацию о версии (мы уже ответили согласием на получение обновлений)
        //Устанавлиеваем состояние UPDATING - мы получаем данные обновлений
        case protocol_state::NEWVERSION:
        {
            if(tag == UMTags::VERSION)
            {
                qDebug() << "UpdaterWorker: HandleHeadersExchange: proccesing!";
                this->HandleVersion();
            }
            else
                throw std::invalid_argument("UpdateWorker: неправильный ответ вместо новой версии!");
            break;
        }
        //Если уже производится обновления
        case protocol_state::UPDATING:
        {
            this->HandleUpdating();
            break;
        }
        default:
        {
            throw std::logic_error("UpdaterWorker: HandleHeadersExchange: неизвестное состояние протокола!");
            emit signalRefreshServerData();
            return;
        }
        } //Switch
    }
}

void UpdaterWorker::HandleUnknown(void)
{
    qDebug() << "UpdaterWorker: HandleUnknown: получено UNET-MES - подключение верно!";
    //И отправляем текущую версию
    ProtocolState = protocol_state::PROPER;
    netfuncs::build_header(header, UMTags::VERSION, version);
    netfuncs::send_header(header, buffer, socket);
}

void UpdaterWorker::HandleProper(void)
{
    //Либо есть обновления "UPDATING"
    if(value == UMTags::SOMEUPDATE)
    {
        qDebug() << "UpdaterWorker: HandleProper: начинаем фоновую загрузку обновлений!";
        ProtocolState = protocol_state::NEWVERSION;

        netfuncs::build_header(header, UMTags::PROTOCOL, UMTags::AGREE);
        netfuncs::send_header(header, buffer, socket);
        return;
    }
    //Либо их нет "NOUPDATE"
    if(value == UMTags::NOUPDATE)
    {
        qDebug() << "UpdaterWorker: HandleProper: обновления не требуются!";
        ProtocolState = protocol_state::COMPLETE;
        //Отправляем также в ответ тэг завершения
        netfuncs::build_header(header, UMTags::PROTOCOL, UMTags::COMPLETE);
        netfuncs::send_header(header, buffer, socket);
        emit signalRefuseUpdates();
        return;
    }
    throw std::invalid_argument("UpdaterWorker: неверный ответ на запрос протокола!");
}

void UpdaterWorker::HandleVersion(void)
{
    qDebug() << "UpdaterWorker: HandleVersion: получена новая версия обновлений: " << value;
    NewVersion = value;
    ProtocolState = protocol_state::UPDATING;

    // Создаем папку в tempdir с названием версии
    QDir versionDir(user_temp_dir.filePath(value));
    if (!versionDir.exists() && !QDir().mkpath(versionDir.path())) {
        qDebug() << "UpdaterWorker: HandleVersion: ошибка при создании папки для новой версии:" << versionDir.path();
        throw excepts::unacceptable("UpdaterWorker: HandleVersion: не удалось создать директорию обновления");
    }

    // Устанавливаем ее как temp_dir
    temp_dir = versionDir;
    qDebug() << "UpdaterWorker: HandleVersion: директория текущих обновлений установлена:" << temp_dir.path();

    // Удаляем всё, что внутри новой директории обновлений
    if(rem_temp_dir_data)
    {
        for (const QFileInfo &entry : temp_dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
            if (entry.isDir()) {
                QDir dir(entry.absoluteFilePath());
                if (!dir.removeRecursively()) {
                    qDebug() << "UpdaterWorker: HandleVersion: не удалось рекурсивно удалить папку:" << dir.path();
                    throw excepts::unacceptable("UpdaterWorker: HandleVersion: не удалось очистить старую папку внутри директории обновлений");
                }
            } else {
                QFile file(entry.absoluteFilePath());
                if (!file.remove()) {
                    qDebug() << "UpdaterWorker: HandleVersion: не удалось удалить файл:" << file.fileName();
                    throw excepts::unacceptable("UpdaterWorker: HandleVersion: не удалось удалить старый файл внутри директории обновлений");
                }
            }
        }
    }
}

// - - - - - - - - Методы для HEADERS EXCHANGE

void UpdaterWorker::HandleUpdating(void)
{
        qDebug() << "UpdaterWorker: HandleUpdating: processing...";
        if(tag == UMTags::NEWDIR)
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got NEWDIR tag...";
            this->HandleNewDir();
            return;
        }
        if(tag == UMTags::NEWFILE)
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got NEWFILE tag...";
            this->HandleNewFile();
            return;
        }
        if(tag == UMTags::DELFILE)
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got DELFILE tag...";
            this->HandleDelFile();
            return;
        }
        if(tag == UMTags::DELDIR)
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got DELDIR tag...";
            this->HandleDelDir();
            return;
        }
        if(tag == UMTags::HASH)
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got HASH tag...";
            this->HandleFileHash();
            return;
        }
        if(tag == UMTags::PROTOCOL && value == UMTags::COMPLETE)
        {
            qDebug() << "UpdaterWorker: HandleUpdating: обновление завершено!";
            ProtocolState = protocol_state::COMPLETE;
            ComType = communication_type::NONE;
            netfuncs::build_header(header, UMTags::PROTOCOL, UMTags::COMPLETE);
            netfuncs::send_header(header, buffer, socket);
            emit signalUpdateReady();
            return;
        }
}

void UpdaterWorker::HandleNewDir(void)
{
    qDebug() << "UpdaterWorker: создаем новый каталог: " << value;
    QString fullPath = QDir(temp_dir).filePath(value);
    QDir dir;
    if (!dir.mkpath(fullPath))
    {
        qDebug() << "UpdaterWorker: ошибка при создании каталога" << fullPath;
        throw std::runtime_error("Ошибка при создании каталога");
    }

    qDebug() << "UpdaterWorker: каталог создан:" << fullPath;
}

void UpdaterWorker::HandleDelFile(void)
{
    qDebug() << "UpdaterWorker: получено указание по удалению файла: " << value;
    //Создаем по указанному пути файл.txt с описанием того, что удаляется
    QString fullPath = QDir(temp_dir).filePath(value);
    QString noticeFile = fullPath + "_" + UMTags::DELFILE + ".txt";

    QFile notice(noticeFile);
    if (notice.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&notice);
        out << UMTags::DELFILE << ":" << value;
        notice.close();
        qDebug() << "UpdaterWorker: создан файл с пометкой на удаление:" << noticeFile;
    } else {
        qDebug() << "UpdaterWorker: HandleDelFile: ошибка создания файла для удаления:" << noticeFile;
        throw std::runtime_error("UpdaterWorker: HandleDelFile: не удалось создать notice-файл удаления");
    }
}

void UpdaterWorker::HandleDelDir(void)
{
    qDebug() << "UpdaterWorker: получено указание по удалению каталога: " << value;

    //Создаем по указанному пути файл.txt с описанием того, что удаляется
    QString fullPath = QDir(temp_dir).filePath(value);
    QString noticeFile = fullPath + "_" + UMTags::DELDIR + ".txt";

    QFile notice(noticeFile);
    if (notice.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&notice);
        out << UMTags::DELDIR << ":" << value;
        notice.close();
        qDebug() << "UpdaterWorker: создан файл с пометкой на удаление:" << noticeFile;
    } else {
        qDebug() << "UpdaterWorker: ошибка создания файла для удаления:" << noticeFile;
        throw std::runtime_error("UpdaterWorker: не удалось создать notice-файл удаления");
    }
}

void UpdaterWorker::HandleNewFile(void)
{
    qDebug() << "UpdaterWorker: получено указание по загрузке файла: " << value;
    //Сохраняем путь файла value и читаем хэш файла
    new_file.setFileName(temp_dir.filePath(value));

    //Если файла нет отправляем согласие, если есть запрашиваем хэш
    QFileInfo new_file_info(new_file);
    if(new_file_info.exists())
    {
        netfuncs::build_header(header, UMTags::NEWFILE, UMTags::HASH);
        netfuncs::send_header(header, buffer, socket);
        return;
    }

    //Если есть, то запросим файл
    netfuncs::build_header(header, UMTags::NEWFILE, UMTags::AGREE);
    netfuncs::send_header(header, buffer, socket);
}

void UpdaterWorker::HandleFileHash(void)
{
    //Определяем условие отказа
    QString file_hash = appfuncs::getFileSHA256(new_file);
    if(file_hash == value)
    {
        qDebug() << "UpdaterWorker: HandleNewFile: file with same path&&hash exists";
        netfuncs::build_header(header, UMTags::NEWFILE, UMTags::REJECT);
        netfuncs::send_header(header, buffer, socket);
        return;
    }

    //В случае согласия
    netfuncs::build_header(header, UMTags::NEWFILE, UMTags::AGREE);
    netfuncs::send_header(header, buffer, socket);
    //Открываем fstream
    filestream.open(new_file.fileName().toStdString(), std::ios::out | std::ios::binary | std::ios::trunc);
    if(!filestream.is_open())
        throw std::runtime_error("UpWorker: HandleNewFile: filestream opening error");

    //Получаем размер файла, устанавливаем total_read, total_left;
    if(socket->bytesAvailable() < 4 && !(socket->waitForReadyRead(50000)))
    {
        filestream.close();
        throw std::runtime_error("UpdateWorker: HandleNewFile: no file weight data");
    }

    quint32 file_weight = netfuncs::get_file_weigth(buffer, socket);
    total_read = 0;
    total_left = file_weight;

    //Переключаем состояние communication_type
    ComType = communication_type::BINARY;
    qDebug() << "UpdaterWorker: HandleNewFile: starts file downloading with " << file_weight << " file weight!";
}

// - - - - - - - - Методы для загрузки файлов

void UpdaterWorker::HandleDownFile(void)
{
    if(ComType == communication_type::BINARY)
    {
        //Смотрим сколько доступно
        //Определяем кол-во чтения из доступного/оставшегося
        if(socket->bytesAvailable() < 0)
            throw std::runtime_error("UpdaterWorker: HandleDownFile: no socket data");
        quint64 bytes_to_read = qMin(total_left, quint64(socket->bytesAvailable()));
        //Вызываем запись из буфера в fstream
        quint64 this_time = netfuncs::ofstream_write(filestream, buffer, bytes_to_read, socket);
        //Изменяем total_read, total_left;
        total_read += this_time;
        total_left -= this_time;

        qDebug() << "UpdaterWorker: HandleDownFile: downloaded " << this_time << " bytes, " << total_read << " total read, " << total_left << " total left";

        //Проверка полностью ли загружен файл
        if(total_left == 0)
        {
            //Если полностью загружен
            //Меняем ComType
            //Закрываем fstream (освобождаем)
            ComType = communication_type::HEADERS;
            filestream.close();

            qDebug() << "UpWorker: HandleDownFile: file downloaded!";
        }
    }
}

// - - - - - - - - Слоты вызываемые при выборе пользователя - - - - - - - - - -
//Слот, который вызывается при отказе от обновления либо его отсутсвии (ответ от сервера)
void UpdaterWorker::RefuseUpdates(void)
{
    //Разрывает соединение
    socket->abort();
    NetState = net_state::OFFLINE;
    ProtocolState = protocol_state::UNKNOWN;
    //Устанавливает таймер (долгий) на повторное соединение
    QTimer::singleShot(UpdateTimeouts::CheckAfterRefuse, this, &UpdaterWorker::RefreshServerData);
    qDebug() << "UpdaterWorker: пользователь отказался от обновления!";
    emit signalOffline();
}
//Слот замены обновлений
//который запустит бинарник замены файлов
void UpdaterWorker::MakeUpdates(void)
{
    qDebug() << "UpdaterWorker: выполняем замену новых файлов!";
    qDebug() << "UpdaterWorker: запускаем процесс Updater!";

    if (!QFile::exists(updater_exe))
    {
        qDebug() << "UpdaterWorker: не обнаружен updater.exe!";
        emit signalNoUpdaterExe();
    }
    else
    {
        qDebug() << "UpdaterWorker: вызываем updater.exe!";
        //Передаем PID основного процесса для отслеживания завершения
        //Передаем новую версию для корректировки реестра
        //Передаем каталог с новой версией
        //Передаем каталог установленной программы
        QProcess *child = new QProcess(this);
        QProcessEnvironment env = appfuncs::set_env(version);

        child->setProcessEnvironment(env);
        child->start(updater_exe.toUtf8().constData());

        //Далее завершаем процесс!!!!
        QApplication::quit();
        //Предусмотреть получение UI сигнала о завершении
    }

}


