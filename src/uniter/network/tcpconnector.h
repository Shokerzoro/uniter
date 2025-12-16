#ifndef TCPCONNECTOR_H
#define TCPCONNECTOR_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QBuffer>
#include <QFile>

namespace netfuncs {

/* Логика работы класса TcpConnector

Два типа взаимодействия - SYMBOLIC/BINARY
Для каждого типа - текущее состояние пакета HEAD/BODY.
Взаимодействует с сокетом/таймером по сигналам и слотам.

Содержит внутренние буферы для каждого состояния (для хранения сырых данных).
Для хранения обработанных SYMBOLIC данных содержит строки.
Для BINARY взаимодействия содержит указатель на QFile.

Для управления пакетизацией хранит переменные (определение конца заголовка/файла)
Для SYMBOLIC: quint16 header_read, header_left;
Для BINARY: quint32 file_read, file_left;

При поступлении данных в сокет void onSockData(void) закачивает их во внутренние буферы SYMBOLIC/BINARY
После чего вызывается обработки основной логики - void virtual process_communication(void);
Основная логика может узавать кол-во данных в буфере, вызывать методаы обработки для HEAD/BODY
Для HEAD виртуальные методы virtual bool process_header_head(void), virtual bool process_binary_head(void)
Для BODY методы bool read_header(void), bool download_file(void).
Переменные управления пакетизацией позволяют методам определить границы чтения и завершенность пакета.

Таймер периодически вызывает обработку основной логики, что позволяет обрабатывать сжатые пакеты.

*/

/* Базовая работа управляющих методов

! === Основная логика process_communication (базовая, может быть переработана)
// Состояние пакета HEAD
Если размер внутреннего буфера >= header_head_size/binary_head_size,
тогда вызываем process_header_head/process_binary_head.
( Если process_header_head/process_binary_head вернул true, тогда вызываем сразу read_header/download_file )
// Состояние пакета BODY
Просто вызываем read_header/download_file.

! === Методы обработки HEAD
process_header_head/process_binary_head читает из внутренних буферов данные header_head_size/binary_head_size
Преобразует в хостовый порядок, устанавливает header_left/file_left, переключает состояние пакета в BODY
Возвращает true в случае успеха

! === Методы обработки BODY
read_header читает в header из буфера минимальное header_left/размер буфера (ограничение интервала сверху).
Из header_left вычитается кол-во действительно прочитанных байтов.
Если header_left == 0, выполняется обработка заголовка (валидация, разделение на tag/value).
Переключение состояния пакета, отправка сигнала, возврат true.

download_file - аналогично, только данные сбрасываются в QFile, отсутствие обработки и
может происходить переключение типа коммуникации в зависимости от sym_after_downloaded

*/

class TcpConnector : public QObject
{
    Q_OBJECT
public:
    // Получает уже существующий (выделенный) сокет
    explicit TcpConnector(QObject* parent = nullptr);
    ~TcpConnector(); //Отписывается от событий
    // Интерфейс
    bool full_cmp(const QString & tag, const QString & value);
    bool tag_cmp(const QString & tag);
    bool value_cmp(const QString & value);
    virtual void reply(const QString & tag, const QString & value);
    virtual void send_data(QFile* new_file = nullptr); // Отправка файла
    QString get_tag(void);
    QString get_value(void);
    // Вызовы для переключения CommunicationType
    void process_symbolic(void); //Меняет CommunicationType на headers
    void process_binary(QFile* new_file); //Меняет CommunicationType на bynary
    // Управление подключением
    void MakeConnect(QString & Ip, quint16 port);
    void MakeDisconnect(void);
    // Изменение настроек (пока что не реализовано)
    void MoveChildenToThread(QThread * NetThread);
    void SetAutoSymAfterBin(bool state);
    void SetAutoBinAfterSym(bool state);
private:
    // Переменные состояния
    enum class CommunicationType { SYMBOLIC, BINARY };
    enum class PacketState { HEAD, BODY };
    CommunicationType ComType;
    PacketState PackState;
    // Основные мемберы
    QTcpSocket* socket = nullptr;
    QTimer* timer = nullptr; //Для периодического вызова read_header/download_file
    // Для SIMBOLIC взаимодействия
    QByteArray symbolic_buffer;
    quint64 sym_offset;
    QString header, tag, value;
    quint16 sym_read, sym_left;
    // Для BINARY взаимодействия
    QByteArray binary_buffer;
    quint64 bin_offset;
    QFile* new_file = nullptr; //не владеет
    quint32 bin_read, bin_left;
    // ====== Вспомогательные методы ======
    static bool ascii_validate(const QString & s);
    // Очищает внутренние буферы и меняет оффсет
    // при превышении заданного лимита
    void purge_buffer(void);
    // Основные методы (просто дополняют буферы и возвращают true
    bool process_sym_head(void); // если получил размер заголовка и меняет на body
    bool process_bin_head(void);
    bool process_sym_body(void); // если получил весь заголовок и меняет на head
    bool process_bin_body(void);
    // Настройки
    unsigned sym_head_size = 2;
    unsigned bin_head_size = 4;
    unsigned max_symbody_size = 256;
    unsigned max_binbody_size = 512*1024*1024;
    unsigned max_sym_buffer = 1024*1024; //1 Мбайт
    unsigned max_bin_buffer = 1024*1024; //1 Мбайт
    bool auto_sym_after_bin = true; // При полной обработке process_sym_body меняет на sybolic
    bool auto_bin_after_sym = false;
protected:
    // Переопределяемые функции для поддержки различных протоколов
    // Основная логика process_communication
    void virtual process_communication(void);
private slots:
    // Инициализирует/чистит переменные
    virtual void onConnected(void);
    void onDisconnected(void);
    // читает все данные в буфер и вызов основной логики process_communication
    void onSockData(void);
    // Вызывает основную логик
    void onTimerAlert(void);
signals:
    void signalSymReady(void);
    void signalBinReady(void);
    void signalConnected(void);
    void signalDisconnected(void);
};

}



#endif // TCPCONNECTOR_H
