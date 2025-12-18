#include "UpdaterWorker.h"
#include "tcpconnector.h"
#include "unetmestags.h"
#include "../common/excepts.h"
#include "../common/appfuncs.h"

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

namespace uniter {
namespace network {

//Базовая инициализация, выполнение соединений
UpdaterWorker::UpdaterWorker(QObject *parent) : QObject{parent}
{
    rem_temp_dir_data = false;
    ServerData = server_data::NONE;
    NetState = net_state::OFFLINE;
    ProtocolState = protocol_state::UNKNOWN;
    port = -1;

    //Динамически выделяем коннектер
    UMConnector = new netfuncs::TcpConnector(this);
    connect(UMConnector, &netfuncs::TcpConnector::signalConnected, this, &UpdaterWorker::OnConnected, Qt::QueuedConnection);
    connect(UMConnector, &netfuncs::TcpConnector::signalDisconnected, this, &UpdaterWorker::onDisconnected, Qt::QueuedConnection);
    connect(UMConnector, &netfuncs::TcpConnector::signalSymReady, this, &UpdaterWorker::onSockData, Qt::QueuedConnection);
    connect(UMConnector, &netfuncs::TcpConnector::signalBinReady, this, &UpdaterWorker::onSockData, Qt::QueuedConnection);

    // Вызов слотов для событийного паттерна
    connect(this, &UpdaterWorker::signalFindConfig, this, &UpdaterWorker::FindConfig, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalGetServerData, this, &UpdaterWorker::GetServerData, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalMakeConnect, this, &UpdaterWorker::MakeConnect, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalRefreshServerData, this, &UpdaterWorker::RefreshServerData, Qt::QueuedConnection);
    connect(this, &UpdaterWorker::signalRefuseUpdates, this, &UpdaterWorker::RefuseUpdates, Qt::QueuedConnection);

    common::appfuncs::AppEnviroment app_env = common::appfuncs::read_env();
    version = app_env.version;
    qDebug() << "UpdaterWorker: текущая версия:" << version;
    user_temp_dir = app_env.temp_dir;
    qDebug() << "UpdaterWorker: директория пользовательских данных установлена:" << user_temp_dir.path();
    config_file = app_env.config;
    updater_exe = app_env.updater_exe;
    recover_exe = app_env.recover_exe;
}


void UpdaterWorker::set_rem_temp_dir_data(bool option)
{
    rem_temp_dir_data = option;
    qDebug() << "UpdaterWorker: rem_temp_dir_data = " << option;
}

void UpdaterWorker::MoveChildenToThread(QThread * NetThread)
{
    UMConnector->moveToThread(NetThread);
    UMConnector->MoveChildenToThread(NetThread);
}

//Основная функция, заканчивающаяся в случае успеха подключением
void UpdaterWorker::StartRun(void)
{
    qDebug() << "UpdaterWorker: StartRun: начал выполнение!";

    if (!QFile::exists(updater_exe))
    {
        qDebug() << "UpdaterWorker: StartRun updater.exe не существует!";
        emit signalNoUpdaterExe();
    }
    if (!QFile::exists(recover_exe))
    {
        qDebug() << "UpdaterWorker: StartRun: recover_exe не существует!";
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
        qDebug() << "UpdaterWorker: FindConfig: INI-файл не найден!";
        emit signalNoServerData(); //Неудача
        //Установка таймера на refresh
        QTimer::singleShot(UpdateTimeouts::findConfigRetryMs, this, &UpdaterWorker::FindConfig);
    }
    else
    {
        qDebug() << "UpdaterWorker: FindConfig: INI-файл найден!";
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
            qDebug() << "UpdaterWorker: GetServerData: найдены данные сервера";
            emit signalMakeConnect();
        }
        else //неудача
        {
            qDebug() << "UpdaterWorker: GetServerData: данные сервера не найдены";
            emit signalNoServerData();
            //Установка таймера на refresh, чтобы заново начать поиск конфига
            QTimer::singleShot(UpdateTimeouts::getServerDataRetryMs, this, &UpdaterWorker::RefreshServerData);
        }
    }
}

//Полное "перечтение" конфигурационного файла
void UpdaterWorker::RefreshServerData(void)
{
    UMConnector->MakeDisconnect();
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
        qDebug() << "UpdaterWorker: MakeConnect: не владеет доступом к файлу конфигурации!";
        emit signalFindConfig();
        return;
    }
    if(ServerData == server_data::FILEEXISTS)
    {
        qDebug() << "UpdaterWorker: MakeConnect: не владеет данными с сервера!";
        emit signalGetServerData();
        return;
    }
    if(ServerData == server_data::FOUND)
    {
        qDebug() << "UpdaterWorker: MakeConnect: попытка подключения к серверу...";
        UMConnector->MakeConnect(IPv4_address, port);
        return;
    }
}

//При соединении выполняет отправку идентификационного хэдера
void UpdaterWorker::OnConnected(void)
{
    qDebug() << "UpdaterWorker: OnConnected: соединен с сервером обновлений!";
    NetState = net_state::ONLINE;
    ProtocolState = protocol_state::UNKNOWN;
    ComType = communication_type::HEADERS;
    emit signalOnline();

    //!!Отправка заголовка "UNET-MES" для инициализации обмена заголовками
    UMConnector->reply(UMTags::PROTOCOL, UMTags::UNETMES);
}

//Основной слот, который проходит по состояниям
void UpdaterWorker::onSockData(void)
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
        catch (common::excepts::unacceptable & un)
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

}

void UpdaterWorker::onDisconnected(void)
{
    if(ProtocolState != protocol_state::COMPLETE)
    {
        qDebug() << "UpdaterWorker: обрыв соединения с сервером!";
        UMConnector->MakeDisconnect();
        if(new_file.isOpen())
            new_file.close();

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

    if(ProtocolState != protocol_state::COMPLETE)
    {
        qDebug() << "UpdaterWorker: HandleHeadersExchange: proccesing!";
        switch (ProtocolState)
        {
        //Обмен типом протокола (уже отправили "PROTOCOL:UNET-MES" со своей стороны)
        //Устанавливаем PROPER состояние протокола и отправляем "VERSION:%version%"
        case protocol_state::UNKNOWN:
        {
            if(UMConnector->full_cmp(UMTags::PROTOCOL, UMTags::UNETMES))
            {
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
            if(UMConnector->tag_cmp(UMTags::PROTOCOL))
            {
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
            if(UMConnector->tag_cmp(UMTags::VERSION))
            {
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
    UMConnector->reply(UMTags::VERSION, version);
}

void UpdaterWorker::HandleProper(void)
{
    //Либо есть обновления "UPDATING"
    if(UMConnector->value_cmp(UMTags::SOMEUPDATE))
    {
        qDebug() << "UpdaterWorker: HandleProper: начинаем фоновую загрузку обновлений!";
        ProtocolState = protocol_state::NEWVERSION;
        UMConnector->reply(UMTags::PROTOCOL, UMTags::AGREE);
        return;
    }
    //Либо их нет "NOUPDATE"
    if(UMConnector->value_cmp(UMTags::NOUPDATE))
    {
        qDebug() << "UpdaterWorker: HandleProper: обновления не требуются!";
        ProtocolState = protocol_state::COMPLETE;
        //Отправляем также в ответ тэг завершения
        UMConnector->reply(UMTags::PROTOCOL, UMTags::COMPLETE);
        emit signalRefuseUpdates();
        return;
    }
    throw std::invalid_argument("UpdaterWorker: неверный ответ на запрос протокола!");
}

void UpdaterWorker::HandleVersion(void)
{
    common::appfuncs::AppEnviroment app_env = common::appfuncs::read_env();
    app_env.new_version = UMConnector->get_value();
    common::appfuncs::set_env(app_env);
    ProtocolState = protocol_state::UPDATING;

    qDebug() << "UpdaterWorker: HandleVersion: получена новая версия обновлений: " << app_env.new_version;

    // Создаем папку в user_temp_dir с названием версии
    temp_dir.setPath(user_temp_dir.filePath(UMConnector->get_value()));
    if (!temp_dir.exists() && !QDir().mkpath(temp_dir.path())) {
        qDebug() << "UpdaterWorker: HandleVersion: ошибка при создании папки для новой версии:" << temp_dir.path();
        throw common::excepts::unacceptable("UpdaterWorker: HandleVersion: не удалось создать директорию обновления");
    }

    qDebug() << "UpdaterWorker: HandleVersion: директория текущих обновлений установлена:" << temp_dir.path();

    // Удаляем всё, что внутри новой директории обновлений
    if(rem_temp_dir_data)
    {
        for (const QFileInfo &entry : temp_dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
            if (entry.isDir()) {
                QDir dir(entry.absoluteFilePath());
                if (!dir.removeRecursively()) {
                    qDebug() << "UpdaterWorker: HandleVersion: не удалось рекурсивно удалить папку:" << dir.path();
                    throw common::excepts::unacceptable("UpdaterWorker: HandleVersion: не удалось очистить старую папку внутри директории обновлений");
                }
            } else {
                QFile file(entry.absoluteFilePath());
                if (!file.remove()) {
                    qDebug() << "UpdaterWorker: HandleVersion: не удалось удалить файл:" << file.fileName();
                    throw common::excepts::unacceptable("UpdaterWorker: HandleVersion: не удалось удалить старый файл внутри директории обновлений");
                }
            }
        }
    }
}

// - - - - - - - - Методы для HEADERS EXCHANGE

void UpdaterWorker::HandleUpdating(void)
{
        qDebug() << "UpdaterWorker: HandleUpdating: processing...";
        if(UMConnector->tag_cmp(UMTags::NEWDIR))
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got NEWDIR tag...";
            this->HandleNewDir();
            return;
        }
        if(UMConnector->tag_cmp(UMTags::NEWFILE))
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got NEWFILE tag...";
            this->HandleNewFile();
            return;
        }
        if(UMConnector->tag_cmp(UMTags::DELFILE))
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got DELFILE tag...";
            this->HandleDelFile();
            return;
        }
        if(UMConnector->tag_cmp(UMTags::DELDIR))
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got DELDIR tag...";
            this->HandleDelDir();
            return;
        }
        if(UMConnector->tag_cmp(UMTags::HASH))
        {
            qDebug() << "UpdaterWorker: HandleUpdating: got HASH tag...";
            this->HandleFileHash();
            return;
        }
        if(UMConnector->full_cmp(UMTags::PROTOCOL, UMTags::COMPLETE))
        {
            qDebug() << "UpdaterWorker: HandleUpdating: обновление завершено!";
            ProtocolState = protocol_state::COMPLETE;
            ComType = communication_type::NONE;
            UMConnector->reply(UMTags::PROTOCOL, UMTags::COMPLETE);
            emit signalUpdateReady();
            return;
        }
}

void UpdaterWorker::HandleNewDir(void)
{
    qDebug() << "UpdaterWorker: создаем новый каталог: " << UMConnector->get_value();
    QString new_dir_path = temp_dir.filePath(UMConnector->get_value());
    QDir dir;
    if (!dir.mkpath(new_dir_path))
    {
        qDebug() << "UpdaterWorker: ошибка при создании каталога" << new_dir_path;
        throw std::runtime_error("Ошибка при создании каталога");
    }

    qDebug() << "UpdaterWorker: каталог создан:" << new_dir_path;
}

void UpdaterWorker::HandleDelFile(void)
{
    qDebug() << "UpdaterWorker: получено указание по удалению файла: " << UMConnector->get_value();
    //Создаем по указанному пути файл.txt с описанием того, что удаляется
    QString del_file_path = temp_dir.filePath(UMConnector->get_value());
    QString notice_file_path = del_file_path + "_" + UMTags::DELFILE + ".txt";

    QFile notice(notice_file_path);
    if (notice.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&notice);
        out << UMTags::DELFILE << ":" << UMConnector->get_value();
        notice.close();
        qDebug() << "UpdaterWorker: создан файл с пометкой на удаление:" << notice_file_path;
    } else {
        qDebug() << "UpdaterWorker: HandleDelFile: ошибка создания файла для удаления:" << notice_file_path;
        throw std::runtime_error("UpdaterWorker: HandleDelFile: не удалось создать notice-файл удаления");
    }
}

void UpdaterWorker::HandleDelDir(void)
{
    qDebug() << "UpdaterWorker: получено указание по удалению каталога: " << UMConnector->get_value();

    //Создаем по указанному пути файл.txt с описанием того, что удаляется
    QString del_dir_path = temp_dir.filePath(UMConnector->get_value());
    QString notice_file_path = del_dir_path + "_" + UMTags::DELDIR + ".txt";

    QFile notice(notice_file_path);
    if (notice.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&notice);
        out << UMTags::DELDIR << ":" << UMConnector->get_value();
        notice.close();
        qDebug() << "UpdaterWorker: создан файл с пометкой на удаление:" << notice_file_path;
    } else {
        qDebug() << "UpdaterWorker: ошибка создания файла для удаления:" << notice_file_path;
        throw std::runtime_error("UpdaterWorker: не удалось создать notice-файл удаления");
    }
}

void UpdaterWorker::HandleNewFile(void)
{
    qDebug() << "UpdaterWorker: получено указание по загрузке файла: " << UMConnector->get_value();
    //Сохраняем путь файла value и читаем хэш файла
    new_file.setFileName(temp_dir.filePath(UMConnector->get_value()));

    //Если файл существует, запрашиваем хэш, если нет, сразу отправляем согласие
    QFileInfo new_file_info(new_file);
    if(new_file_info.exists())
    {
        UMConnector->reply(UMTags::NEWFILE, UMTags::HASH);
        return;
    }

    //Если есть, то запросим файл
    new_file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate | QIODeviceBase::Unbuffered);
    if(!new_file.isOpen())
        throw std::runtime_error("UpdateWorker: HandleNewFile: error openning file");

    UMConnector->process_binary(&new_file);
    ComType = communication_type::BINARY;
    UMConnector->reply(UMTags::NEWFILE, UMTags::AGREE);

    qDebug() << "UpdaterWorker: HandleNewFile: starts file downloading!";
}

void UpdaterWorker::HandleFileHash(void)
{
    //Определяем условие отказа
    QString file_hash = common::appfuncs::getFileSHA256(new_file);
    if(file_hash == UMConnector->get_value())
    {
        qDebug() << "UpdaterWorker: HandleNewFile: file with same path&&hash exists";
        UMConnector->reply(UMTags::NEWFILE, UMTags::REJECT);
        return;
    }

    //В случае согласия
    //Открываем fstream
    new_file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate | QIODeviceBase::Unbuffered);
    if(!new_file.isOpen())
        throw std::runtime_error("UpdateWorker: HandleNewFile: error openning file");

    UMConnector->process_binary(&new_file);
    ComType = communication_type::BINARY;
    UMConnector->reply(UMTags::NEWFILE, UMTags::AGREE);

    qDebug() << "UpdaterWorker: HandleNewFile: starts file downloading!";
}

// - - - - - - - - Методы для загрузки файлов

void UpdaterWorker::HandleDownFile(void)
{
    if(ComType == communication_type::BINARY)
    {
        //файл полностью загружен
        new_file.close();
        ComType = communication_type::HEADERS;
        UMConnector->process_symbolic();
        UMConnector->reply(UMTags::NEWFILE, UMTags::COMPLETE);

        qDebug() << "UpWorker: HandleDownFile: file downloaded!";
    }
}

// - - - - - - - - Слоты вызываемые при выборе пользователя - - - - - - - - - -
//Слот, который вызывается при отказе от обновления либо его отсутсвии (ответ от сервера)
void UpdaterWorker::RefuseUpdates(void)
{
    //Разрывает соединение
    UMConnector->MakeDisconnect();
    NetState = net_state::OFFLINE;
    ProtocolState = protocol_state::UNKNOWN;
    ComType = communication_type::NONE;
    //Устанавливает таймер (долгий) на повторное соединение
    QTimer::singleShot(UpdateTimeouts::CheckAfterRefuse, this, &UpdaterWorker::RefreshServerData);
    qDebug() << "UpdaterWorker: пользователь отказался от обновления!";
    emit signalOffline();
    common::appfuncs::write_log("UpdaterWorker: update rejected");
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
        common::appfuncs::write_log("UpdaterWorker: starting updater.exe");
        common::appfuncs::write_log(QString("current version: " + version + " new version: " + NewVersion));
        common::appfuncs::log_time();

        QProcess *updater = new QProcess(this);
        updater->start(updater_exe.toUtf8().constData());

        //Далее завершаем процесс!!!!
        QApplication::quit();
    }

}

} // network
} // uniter

