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
    //Загрузка данных из конфига и первая попытка подключения
    void StartRun(void);
    //Тут нужно написать деструктор
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
    QString updater_exe;
    QDir temp_dir;

    //Сетевые переменные
    QString header, tag, value;
    std::vector<char> buffer;
    QTcpSocket *socket;

    //Приватные методы отработки протокола
    void HandleUnknown(void);
    void HandleProper(const QString & value);
    void HandleVersion(const QString & value);

    void HandleNewDir(const QString & value);
    void HandleNewFile(const QString & value);
    void HandleDelFile(const QString & value);
    void HandleDelDir(const QString & value);

signals:
    //Для событийного паттерна
    void signalFindConfig();
    void signalGetServerData();
    void signalMakeConnect();
    void signalMoreSocketData();
    void signalNetError();
    void signalRefreshServerData();
    void signalRefuse();

    //Внешние сигналы
    void NoServerData(void); //Сигнал, что не удалось найти данные сервера
    void Online(void);
    void Offline(void);
    void SomeUpdate(void); //Запрос на обновление пользователя

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
    void MakeUpdates(void); //Запуск бинарника по замене файлов
    void Refuse(void); //Завершение потока
};

#endif // UPDATERWORKER_H
