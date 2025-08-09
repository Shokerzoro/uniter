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
#include <QCryptographicHash>
#include <vector>
#include <exception>

//Базовая инициализация, выполнение соединений
UpdaterWorker::UpdaterWorker(QObject *parent) : QObject{parent}
{
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

    //Вызыаем сразу слот на попытку подключения
    emit signalSetVersion(version);
    emit signalOffline();
    emit signalMakeConnect();
}

//Метод установки соединения с сервером
void UpdaterWorker::MakeConnect()
{
    if(ServerData == server_data::NONE)
    {
        qDebug() << "UpdaterWorker не владеет доступом к файлу конфигурации!";
        emit signalFindConfig();
    }
    if(ServerData == server_data::FILEEXISTS)
    {
        qDebug() << "UpdaterWorker не владеет данными с сервера!";
        emit signalGetServerData();
    }
    if(ServerData == server_data::FOUND)
    {
        if (socket->state() == QAbstractSocket::UnconnectedState)
        {
            qDebug() << "UpdaterWorker: попытка подключения к серверу...";
            socket->connectToHost(IPv4_address, port);
        } else {
            qDebug() << "Сокет уже подключён или подключается";
        }
    }
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

//При соединении выполняет отправку идентификационного хэдера
void UpdaterWorker::OnConnected(void)
{
    qDebug() << "UpdaterWorker: соединен с сервером обновлений!";
    NetState = net_state::ONLINE;
    emit signalOnline();

    //!!Отправка заголовка "UNET-MES"
    try {
        header = netfuncs::build_header(UMTags::PROTOCOL, UMTags::UNETMES);
        netfuncs::send_header(header, buffer, socket);
    }
    catch (std::runtime_error & ex)
    {
        qDebug() << "UpdaterWorker: ошибка при отправке заголовка!";
        emit signalNetError();
    }
}

//Слот, вызывающийся при ошибках, на 99% означающих отключение сервера
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
        NetState = net_state::OFFLINE;
        ProtocolState = protocol_state::UNKNOWN;
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
        NetState = net_state::OFFLINE;
        ProtocolState = protocol_state::UNKNOWN;
        emit signalOffline();
        //Постановка небольшого таймера на реконнект
        QTimer::singleShot(UpdateTimeouts::netReconnectMs, this, &UpdaterWorker::MakeConnect);
    }
}

//Основной слот по сути, выполняющий загрузку обновлений и их сохранение
void UpdaterWorker::GotSockData(void)
{
if(ProtocolState != protocol_state::COMPLETE && socket->bytesAvailable() != 0)
{
    try { //Try

        //Читаем хэдер, даже если прислали файл - хэдер это покажет
        netfuncs::read_header(header, buffer, socket);
        netfuncs::parse_header(header, tag, value);

        switch (ProtocolState)
        {
            //Обмен типом протокола (уже отправили "PROTOCOL:UNET-MES" со своей стороны)
            //Устанавливаем PROPER состояние протокола и отправляем "VERSION:%version%"
            case protocol_state::UNKNOWN:
                if(tag == UMTags::PROTOCOL && value == UMTags::UNETMES)
                    this->HandleUnknown();
                else
                    throw std::invalid_argument("UpdaterWorker: ошибка идентификации протокола...");
            break;

            //Получаем информацию о наличии обновлений или отсутствие "PROTOCOL:SOMEUPDATE" / "PROTOCOL:NOUPDATE"
            //Отправляем в ответ согласие или не согласие на получение если они есть и устанавливаем состояние NEWVERSION
            //Что означает, что мы ждем получание новой версии
            case protocol_state::PROPER:
                if(tag == UMTags::PROTOCOL)
                        this->HandleProper(value);
                else
                    throw std::invalid_argument("UpdateWorker: неправильный ответ на запрос о наличии обновления!");
            break;

            //Получаем информацию о версии (мы уже ответили согласием на получение обновлений)
            //Устанавлиеваем состояние SOMEUPDATE - мы получаем данные обновлений
            case protocol_state::NEWVERSION:
                if(tag == UMTags::VERSION)
                    this->HandleVersion(value);
                else
                    throw std::invalid_argument("UpdateWorker: неправильный ответ вместо новой версии!");
            break;

            //
            case protocol_state::SOMEUPDATE:
                if(tag == UMTags::NEWDIR)
                {
                    this->HandleNewDir(value);
                    break;
                }
                if(tag == UMTags::NEWFILE)
                {
                    this->HandleNewFile(value);
                    break;
                }
                if(tag == UMTags::DELFILE)
                {
                    this->HandleDelFile(value);
                    break;
                }
                if(tag == UMTags::DELDIR)
                {
                    this->HandleDelDir(value);
                    break;
                }
                if(tag == UMTags::PROTOCOL && value == UMTags::COMPLETE)
                {
                    qDebug() << "UpdaterWorker: обновление завершено!";
                    ProtocolState = protocol_state::COMPLETE;
                    header = netfuncs::build_header(UMTags::PROTOCOL, UMTags::COMPLETE);
                    netfuncs::send_header(header, buffer, socket);
                    emit signalUpdateReady();
                    break;
                }
                throw std::invalid_argument("UpdateWorker: неизвестный тэг при загрузке обновлений!");
            break;

            default:
                qDebug() << "UpdaterWorker: неизвестное состояние протокола!";
                emit signalRefreshServerData();
                return;

        } //Switch

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

    //Проверка наличия необработанных данных
    if(socket->bytesAvailable())
        emit signalMoreSocketData();
    return;
}
}

// - - - - - - - - Методы для обработки различных состояний протокола - - - -
void UpdaterWorker::HandleUnknown(void)
{
    qDebug() << "UpdaterWorker: получено UNET-MES - подключение верно!";
    //И отправляем текущую версию
    ProtocolState = protocol_state::PROPER;
    header = netfuncs::build_header(UMTags::VERSION, version);
    netfuncs::send_header(header, buffer, socket);
}

void UpdaterWorker::HandleProper(const QString & value)
{
    //Либо есть обновления "SOMEUPDATE"
    if(value == UMTags::SOMEUPDATE)
    {
        qDebug() << "UpdaterWorker: получено SOMEUPDATE!";
        qDebug() << "UpdaterWorker: начинаем фоновую загрузку обновлений!";
        ProtocolState = protocol_state::NEWVERSION;

        header = netfuncs::build_header(UMTags::PROTOCOL, UMTags::AGREE);
        netfuncs::send_header(header, buffer, socket);
        return;
    }
    //Либо их нет "NOUPDATE"
    if(value == UMTags::NOUPDATE)
    {
        qDebug() << "UpdaterWorker: получено NOUPDATE!";
        ProtocolState = protocol_state::COMPLETE;
        //Отправляем также в ответ тэг завершения
        header = netfuncs::build_header(UMTags::PROTOCOL, UMTags::COMPLETE);
        netfuncs::send_header(header, buffer, socket);
        emit signalRefuseUpdates();
        return;
    }
    throw std::invalid_argument("UpdaterWorker: неверный ответ на запрос протокола!");
}

//Создаем во временной папке новый каталог и сохраняем версию
void UpdaterWorker::HandleVersion(const QString & value)
{
    qDebug() << "UpdaterWorker: получаю новую версию обновлений: " << value;
    NewVersion = value;
    ProtocolState = protocol_state::SOMEUPDATE;

    // Создаем папку в tempdir с названием версии
    // Вот здесь при переподключениях постоянно пересоздается вложенная папка !!LOGICERROR
    QDir versionDir(user_temp_dir.filePath(value));
    if (!versionDir.exists() && !QDir().mkpath(versionDir.path())) {
        qDebug() << "UpdaterWorker: ошибка при создании папки для новой версии:" << versionDir.path();
        throw excepts::unacceptable("UpdaterWorker: не удалось создать директорию обновления");
    }

    // Устанавливаем ее как temp_dir
    temp_dir = versionDir;
    qDebug() << "UpdaterWorker: директория текущих обновлений установлена:" << temp_dir.path();

    // Удаляем всё, что внутри новой директории обновлений
#ifdef REM_TEMP_UPDATA
    for (const QFileInfo &entry : temp_dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        if (entry.isDir()) {
            QDir dir(entry.absoluteFilePath());
            if (!dir.removeRecursively()) {
                qDebug() << "UpdaterWorker: не удалось рекурсивно удалить папку:" << dir.path();
                throw excepts::unacceptable("UpdaterWorker: не удалось очистить старую папку внутри директории обновлений");
            }
        } else {
            QFile file(entry.absoluteFilePath());
            if (!file.remove()) {
                qDebug() << "UpdaterWorker: не удалось удалить файл:" << file.fileName();
                throw excepts::unacceptable("UpdaterWorker: не удалось удалить старый файл внутри директории обновлений");
            }
        }
    }
#endif
}

//Создаем во временной папке новый каталог
void UpdaterWorker::HandleNewDir(const QString & value)
{
    qDebug() << "UpdaterWorker: создаем новый каталог: " << value;
    QString fullPath = QDir(temp_dir).filePath(value);
    QDir dir;
    if (!dir.mkpath(fullPath)) {
        qDebug() << "UpdaterWorker: ошибка при создании каталога" << fullPath;
        throw std::runtime_error("Ошибка при создании каталога");
    }

    qDebug() << "UpdaterWorker: каталог создан:" << fullPath;
}

//Сохраняем файл, если его нет
void UpdaterWorker::HandleNewFile(const QString & value)
{
    qDebug() << "UpdaterWorker: обработка нового файла: " << value;
    QString filePath = QDir(temp_dir).filePath(value);
    QFileInfo fi(filePath);
    QString server_hash;

    //Получаем хэш
    netfuncs::read_header(header, buffer, socket);
    netfuncs::parse_header(header, tag, server_hash);
    if(tag != UMTags::HASH)
        throw std::invalid_argument("UpdaterWorker: не получен хэш файла");

    if (fi.exists())
    {

        // Тут получение хэша
        QString hash = this->getFileSHA256(filePath);

        qDebug() << "UpdaterWorker: файл уже существует, сравниваем хэш:" << hash;

        if(hash == server_hash) //Если хэш совпадает, то высылаем отказ
        {
            qDebug() << "UpdaterWorker: файл уже существует и соответсвует обновлению: " << filePath;
            header = netfuncs::build_header(UMTags::NEWFILE, UMTags::REJECT);
            netfuncs::send_header(header, buffer, socket);
            return;
        }
    }

    qDebug() << "UpdaterWorker: необходима загрузка, подтверждаю получение:" << filePath;
    header = netfuncs::build_header(UMTags::NEWFILE, UMTags::AGREE);
    netfuncs::send_header(header, buffer, socket);

    //Скармливаем 4 байта размера и остальное body функции загрузки
    netfuncs::download_file(filePath, socket);
    //Подтверждаем успешное выполнение
    header = netfuncs::build_header(UMTags::NEWFILE, UMTags::COMPLETE);
    netfuncs::send_header(header, buffer, socket);
}

void UpdaterWorker::HandleDelFile(const QString & value)
{
    qDebug() << "UpdaterWorker: получено указание по удалению файла: " << value;

    //Создаем по указанному пути файл.txt с описанием того, что удаляется
    QString fullPath = QDir(temp_dir).filePath(value);
    QString noticeFile = fullPath + UMTags::DELFILE + ".txt";

    QFile notice(noticeFile);
    if (notice.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&notice);
        out << UMTags::DELFILE << ":" << value;
        notice.close();
        qDebug() << "UpdaterWorker: создан файл с пометкой на удаление:" << noticeFile;
    } else {
        qDebug() << "UpdaterWorker: ошибка создания файла для удаления:" << noticeFile;
        throw std::runtime_error("UpdaterWorker: не удалось создать notice-файл удаления");
    }
}

void UpdaterWorker::HandleDelDir(const QString & value)
{
    qDebug() << "UpdaterWorker: получено указание по удалению каталога: " << value;

    //Создаем по указанному пути файл.txt с описанием того, что удаляется
    QString fullPath = QDir(temp_dir).filePath(value);
    QString noticeFile = fullPath + UMTags::DELDIR + ".txt";

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

//Приватные методы вспомогательные для отработки протокола
QString UpdaterWorker::getFileSHA256(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Не удалось открыть файл для чтения при хэшировании");

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
        hash.addData(file.read(4096));

    return hash.result().toHex();
}

// - - - - - - - - Слоты вызываемые от выбора пользователя - - - - - - - - - -
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
    emit signalOnline();
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


