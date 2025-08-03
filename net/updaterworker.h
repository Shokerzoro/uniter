#ifndef UPDATERWORKER_H
#define UPDATERWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QDir>
#include <vector>

class UpdaterWorker : public QObject
{
    Q_OBJECT
public:
    //Вызывает конструкторы сокета и устанавливает состояния
    explicit UpdaterWorker(QObject *parent = nullptr);
    ~UpdaterWorker(void);
    //Загрузка данных из конфига и первая попытка подключения
    void StartRun(void);
private:
    //Приватные классы состояний
    enum class server_data { NONE, FILEEXISTS, FOUND };
    enum class net_state { OFFLINE, ONLINE };
    enum class protocol_state { UNKNOWN, PROPER, NEWVERSION, SOMEUPDATE, COMPLETE };

    //Переменные состояний и сетевые данные
    server_data ServerData;
    net_state NetState;
    protocol_state ProtocolState;
    QString IPv4_address;
    int port;

    //Системные переменные
    QString version, NewVersion;
    QDir basePath;
    QString config_file;
    QString updater_exe, recover_exe;
    QDir temp_dir;

    //Сетевые переменные
    QString header, tag, value;
    std::vector<char> buffer;
    QTcpSocket* socket;

    //Приватные методы отработки протокола
    void HandleUnknown(void);
    void HandleProper(const QString & value);
    void HandleVersion(const QString & value);
    void HandleNewDir(const QString & value);
    void HandleNewFile(const QString & value);
    void HandleDelFile(const QString & value);
    void HandleDelDir(const QString & value);

    //Приватные методы вспомогательные для отработки протокола
    QString getFileSHA256(const QString &filePath);
signals:
    //Для событийного паттерна
    void signalFindConfig(void);
    void signalGetServerData(void);
    void signalMakeConnect(void);
    void signalMoreSocketData(void);
    void signalNetError(void);
    void signalRefreshServerData(void);
    void signalRefuseUpdates(void);

    //Внешние сигналы
    void signalNoUpdaterExe(void);
    void signalNoRecoverExe(void);
    void signalNoServerData(void); //Сигнал, что не удалось найти данные сервера
    void signalOnline(void);
    void signalOffline(void);
    void signalUpdateReady(void); //Запрос на обновление пользователя

public slots:
    //Слоты для взаимодействия с сокетом
    void OnConnected(void); //Переключает в онлайн
    void OnNetError(void); //Переключает в офлайн
    void Disconnected(void); //Также, но по другой причине
    void GotSockData(void); //Есть данные для чтения

    //Слоты для внешнего взаимодействия (с графическим интерфейсом)
    void FindConfig(void);
    void GetServerData(void);
    void RefreshServerData(void); //Полное "перечтение" конфигурационного файла
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
};

#endif // UPDATERWORKER_H
