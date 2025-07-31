#include "UpdaterWorker.h"
#include "netfunc.h"
#include "../exeptions/unacceptable.h"

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
    connect(this, &UpdaterWorker::signalRefuse, this, &UpdaterWorker::Refuse, Qt::QueuedConnection);

    //Динамически выделяем сокет
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &UpdaterWorker::OnConnected);
    connect(socket, &QTcpSocket::readyRead, this, &UpdaterWorker::GotSockData);
    connect(socket, &QTcpSocket::disconnected, this, &UpdaterWorker::Disconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &UpdaterWorker::OnNetError);

    basePath = QCoreApplication::applicationDirPath();
    version = QCoreApplication::applicationVersion();
    temp_dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    config_file = basePath.filePath("config/config.ini");

    updater_exe = basePath.filePath("bin/updater.exe");
    if (!QFile::exists(updater_exe))
    {
        qDebug() << "UpdaterWorker: не обнаружен updater.exe!";
        //qDebug() << "UpdaterWorker: завершение работы!";
        //disconnect();
        //this->deleteLater();
    }

    qDebug() << "UpdaterWorker создан!";
}

//Основная функция, заканчивающаяся в случае успеха подключением
void UpdaterWorker::StartRun(void)
{
    qDebug() << "UpdaterWorker начал выполнение!";
    //Вызыаем сразу слот на попытку подключения
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
        emit NoServerData(); //Неудача
        //Установка таймера на refresh
        QTimer::singleShot(30 * 1000, this, &UpdaterWorker::FindConfig); // 1000 мс = 1 секунда
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
            emit NoServerData();
            //Установка таймера на refresh, чтобы заново начать поиск конфига
            QTimer::singleShot(30 * 1000, this, &UpdaterWorker::RefreshServerData); // 1000 мс = 1 секунда
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
    emit Offline();
    //Запуск цикла по сути с начальной точки
    emit signalMakeConnect();
}

//При соединении выполняет отправку идентификационного хэдера
void UpdaterWorker::OnConnected(void)
{
    qDebug() << "UpdaterWorker: соединен с сервером обновлений!";
    NetState = net_state::ONLINE;
    emit Online();

    //!!Отправка заголовка "UNET-MES"
    try {
        header = QString(TagStrings::PROTOCOL) + ":" + QString(TagStrings::UNETMES);
        send_header(header, buffer, socket);
    }
    catch (std::runtime_error & ex)
    {
        qDebug() << "UpdaterWorker: ошибка при отправке заголовка!";
        emit signalNetError();
    }
}

//Слот, вызывающийся при ошибках, на 99% означающих отключение сервера
//Будет срабатывать только при работе сокета, т.к. в промежутках сокет закрыт
//Поэтому не перекроет таймер в Refuse
void UpdaterWorker::OnNetError(void)
{
    qDebug() << "UpdaterWorker: ошибка TCP соединения!";
    qDebug() << "Error string:" << socket->errorString();

    if(socket->state() == QAbstractSocket::ConnectedState)
        socket->abort();
    NetState = net_state::OFFLINE;
    ProtocolState = protocol_state::UNKNOWN;
    emit Offline();
    //Постановка небольшого таймера на реконнект
    QTimer::singleShot(10 * 1000, this, &UpdaterWorker::MakeConnect); // 1000 мс = 1 секунда
}

void UpdaterWorker::Disconnected(void)
{
    qDebug() << "UpdaterWorker: обрыв соединения с сервером!";
    NetState = net_state::OFFLINE;
    ProtocolState = protocol_state::UNKNOWN;
    emit Offline();
    //Постановка небольшого таймера на реконнект
    QTimer::singleShot(10 * 1000, this, &UpdaterWorker::MakeConnect); // 1000 мс = 1 секунда
}

//Основной слот по сути, выполняющий загрузку обновлений и их сохранение
void UpdaterWorker::GotSockData(void)
{
    try { //Try

        //Читаем хэдер, даже если прислали файл - хэдер это покажет
        read_header(header, buffer, socket);
        parse_header(header, tag, value);

        switch (ProtocolState)
        {
            //Обмен типом протокола
            case protocol_state::UNKNOWN:
                if(tag == TagStrings::PROTOCOL && value == TagStrings::UNETMES)
                    this->HandleUnknown();
                else
                    throw std::invalid_argument("UpdaterWorker: ошибка идентификации протокола...");
            break;

            //Получаем информацию о наличии обновлений
            case protocol_state::PROPER:
                if(tag == TagStrings::PROTOCOL)
                        this->HandleProper(value);
                else
                    throw std::invalid_argument("UpdateWorker: неправильный ответ на запрос о наличии обновления!");
            break;

            //Получаем информацию о версии
            case protocol_state::NEWVERSION:
                if(tag == TagStrings::VERSION)
                    this->HandleVersion(value);
                else
                    throw std::invalid_argument("UpdateWorker: неправильный ответ вместо новой версии!");
            break;

            case protocol_state::SOMEUPDATE:
                if(tag == TagStrings::NEWDIR)
                {
                    this->HandleNewDir(value);
                    break;
                }
                if(tag == TagStrings::NEWFILE)
                {
                    this->HandleNewFile(value);
                    break;
                }
                if(tag == TagStrings::DELFILE)
                {
                    this->HandleDelFile(value);
                    break;
                }
                if(tag == TagStrings::DELDIR)
                {
                    this->HandleDelDir(value);
                    break;
                }
                if(tag == TagStrings::PROTOCOL && value == TagStrings::COMPLETE)
                {
                    qDebug() << "UpdaterWorker: обновление завершено!";
                    emit SomeUpdate();
                }
                throw std::invalid_argument("UpdateWorker: неизвестный тэг при загрузке обновлений!");

            default:
                qDebug() << "UpdaterWorker: неизвестное состояние протокола!";
                emit signalRefreshServerData();
                return;

        } //Switch

    } //Try
    catch (Unacceptable & un)
    {
        qDebug() << "UpdaterWorker: недопустимое исключние!";
        qDebug() << un.what();
        //Остановка воркера
        emit Offline();
        disconnect();
        this->deleteLater();
    }
    catch (std::runtime_error & ex)
    {
        qDebug() << "UpdaterWorker: ошибка времени выполненя!";
        qDebug() << ex.what();
        emit signalNetError();
        return;
    }
    catch(std::invalid_argument & ex)
    {
        qDebug() << "UpdaterWorker: ошибка протокола или неверные данные!";
        qDebug() << ex.what();
        emit signalRefreshServerData();
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

// - - - - - - - - Методы для обработки различных состояний протокола - - - -
void UpdaterWorker::HandleUnknown(void)
{
    qDebug() << "UpdaterWorker: получено UNET-MES - подключение верно!";
    //И отправляем текущую версию
    ProtocolState = protocol_state::PROPER;
    send_header(TagStrings::GETUPDATE, buffer, socket);
    send_header(version, buffer, socket);
}

void UpdaterWorker::HandleProper(const QString & value)
{
    //Либо есть обновления "SOMEUPDATE"
    if(value == TagStrings::SOMEUPDATE)
    {
        qDebug() << "UpdaterWorker: получено SOMEUPDATE!";
        qDebug() << "UpdaterWorker: начинаем фоновую загрузку обновлений!";
        ProtocolState = protocol_state::NEWVERSION;

        header = QString(TagStrings::PROTOCOL) + ":" + QString(TagStrings::AGREE);
        send_header(header, buffer, socket);
        return;
    }
    //Либо их нет "NOUPDATE"
    if(value == TagStrings::NOUPDATE)
    {
        qDebug() << "UpdaterWorker: получено NOUPDATE!";
        emit signalRefuse();
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

    //Создаем папку в tempdir c названием версии
    QDir versionDir(QDir(temp_dir).filePath(value));
    if (!versionDir.exists() && !QDir().mkpath(versionDir.path())) {
        qDebug() << "UpdaterWorker: ошибка при создании папки для новой версии:" << versionDir.path();
        throw Unacceptable("UpdaterWorker: не удалось создать директорию обновления");
    }
    //Устанавливаем ее как tempdir
    temp_dir = versionDir.path(); // Устанавливаем новую temp_dir
    qDebug() << "UpdaterWorker: директория обновлений установлена:" << temp_dir;
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
    qDebug() << "UpdaterWorker: получаем новый файл: " << value;
    QString filePath = QDir(temp_dir).filePath(value);
    QFileInfo fi(filePath);

    if (fi.exists()) {
        qDebug() << "UpdaterWorker: файл уже существует, не будем перезаписывать:" << filePath;
        //Тут в идеале получить хэш этого файла, и отправить на сравнение
        //Либо гарантировать сохранение файла только при полном его получении, а в противном случае удалять

        header = QString(TagStrings::NEWFILE) + ":" + QString(TagStrings::REJECT);
        send_header(header, buffer, socket);
        return;
    }

    qDebug() << "UpdaterWorker: файл не существует, готов к получению:" << filePath;
    header = QString(TagStrings::NEWFILE) + ":" + QString(TagStrings::AGREE);
    send_header(header, buffer, socket);

    //Скармливаем 4 байта размера и body функции загрузки
    download_file(filePath, socket);
}

void UpdaterWorker::HandleDelFile(const QString & value)
{
    qDebug() << "UpdaterWorker: получено указание по удалению файла: " << value;

    //Создаем по указанному пути файл.txt с описанием того, что удаляется
    QString fullPath = QDir(temp_dir).filePath(value);
    QString noticeFile = fullPath + TagStrings::DELFILE + ".txt";

    QFile notice(noticeFile);
    if (notice.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&notice);
        out << TagStrings::DELFILE;
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
    QString noticeFile = fullPath + TagStrings::DELDIR + ".txt";

    QFile notice(noticeFile);
    if (notice.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&notice);
        out << TagStrings::DELDIR;
        notice.close();
        qDebug() << "UpdaterWorker: создан файл с пометкой на удаление:" << noticeFile;
    } else {
        qDebug() << "UpdaterWorker: ошибка создания файла для удаления:" << noticeFile;
        throw std::runtime_error("UpdaterWorker: не удалось создать notice-файл удаления");
    }
}

// - - - - - - - - Слоты вызываемые от выбора пользователя - - - - - - - - - -
//Слот, который вызывается при отказе от обновления либо его отсутсвии (ответ от сервера)
void UpdaterWorker::Refuse(void)
{
    //Разрывает соединение
    socket->abort();
    NetState = net_state::OFFLINE;
    ProtocolState = protocol_state::UNKNOWN;
    //Устанавливает таймер (долгий) на повторное соединение
    QTimer::singleShot(24*60*60*1000, this, &UpdaterWorker::MakeConnect); // 1000 мс = 1 секунда
    //После этого будет опять отрабатывать основной слот
}
//Слот замены обновлений.
void UpdaterWorker::MakeUpdates(void)
{
    qDebug() << "UpdaterWorker: выполняем замену новых файлов!";
    qDebug() << "UpdaterWorker: запускаем процесс Updater!";

    //который запустит бинарник замены файлов
    //Сначала определяем PID и запускаем бинарник с этой переменной окружения
    //Также передаем версию для внесения в реестр
    QProcess *child = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PARENT_PID", QString::number(QCoreApplication::applicationPid()));
    env.insert("VERSION", version);
    child->setProcessEnvironment(env);
    child->start(updater_exe.toUtf8().constData());

    //Далее завершаем процесс!!!!
    QCoreApplication::exit(0);
    //Предусмотреть получение UI сигнала о завершении
}


