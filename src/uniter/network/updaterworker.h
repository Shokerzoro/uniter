#ifndef UPDATERWORKER_H
#define UPDATERWORKER_H

#include "tcpconnector.h"

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QDir>
#include <QThread>
#include <vector>
#include <fstream>

class UpdaterWorker : public QObject
{
    Q_OBJECT
public:
    //Вызывает конструкторы сокета и устанавливает состояния
    explicit UpdaterWorker(QObject *parent = nullptr);
    //Загрузка данных из конфига и первая попытка подключения
    void StartRun(void);
    //Настройки
    void set_rem_temp_dir_data(bool option); // удаление всех файлов во временной папке в перед загрузкой
    void MoveChildenToThread(QThread * NetThread);
private:
    //Настройки
    bool rem_temp_dir_data;

    //Приватные классы состояний
    enum class server_data { NONE, FILEEXISTS, FOUND };
    enum class net_state { OFFLINE, ONLINE };
    enum class protocol_state { UNKNOWN, PROPER, NEWVERSION, UPDATING, COMPLETE };
    enum class communication_type { NONE, HEADERS, BINARY };

    //Переменные состояний
    server_data ServerData;
    net_state NetState;
    protocol_state ProtocolState;
    communication_type ComType;
    //Сетевые данные
    netfuncs::TcpConnector* UMConnector;
    QString IPv4_address;
    int port;
    //Системные переменные
    QString version, NewVersion;
    QString config_file, updater_exe, recover_exe;
    QDir user_temp_dir, temp_dir;
    QFile new_file;

    //Приватные методы отработки протоколов
    void HandleUnknown(void);
    void HandleProper(void);
    void HandleVersion(void);
    void HandleUpdating(void);
    //Методы обработки UPDATING STATE
    void HandleHeadersExchange(void);
    void HandleDownFile(void);
    //Методы обработки HEADERS EXCHANGE
    void HandleNewDir(void);
    void HandleNewFile(void);
    void HandleDelFile(void);
    void HandleDelDir(void);
    void HandleFileHash(void);
signals:
    // Вызов слотов для событийного паттерна
    void signalFindConfig(void);
    void signalGetServerData(void);
    void signalRefreshServerData(void);
    void signalMakeConnect(void);
    void signalRefuseUpdates(void);

    //Внешние сигналы
    void signalNoUpdaterExe(void);
    void signalNoRecoverExe(void);
    void signalNoServerData(void); //Сигнал, что не удалось найти данные сервера
    void signalOnline(void);
    void signalOffline(void);
    void signalUpdateReady(void); //Запрос на обновление пользователя
    //Сигнал для Labal, потом удалить
    void signalSetVersion(const QString & version);

public slots:
    //Слоты для взаимодействия с сокетом
    void OnConnected(void); //Переключает в онлайн
    void onDisconnected(void); //Также, но по другой причине
    void onSockData(void); //Есть данные для чтения

    //Слоты для внешнего взаимодействия (с графическим интерфейсом)
    void FindConfig(void);
    void GetServerData(void);
    void RefreshServerData(void);
    void MakeConnect(void);

    // Слоты для согласия / отказа выполнить обновления
    void MakeUpdates(void); //Запуск бинарника по замене файлов
    void RefuseUpdates(void); //Завершение потока
};

struct UpdateTimeouts { // 1000 мс = 1 секунда
    static constexpr int CheckAfterRefuse = 24*60*60*1000;
    static constexpr int findConfigRetryMs = 30 * 1000;
    static constexpr int getServerDataRetryMs = 30 * 1000;
    static constexpr int netReconnectMs = 10 * 1000;
    static constexpr int fileHeadMS = 10 * 1000;
};

#endif // UPDATERWORKER_H
