#include "excepts.h"
#include "netfunc.h"

#include <QTcpSocket>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QtEndian>
#include <QDebug>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <string>

#define MAX_HEADER_SIZE 255

namespace netfuncs {

// Внутренний класс для валидации ASCII-строки
ascii_string::ascii_string(const std::string & input) : std::string(input) { validate(); }
ascii_string::ascii_string(const char* input) : std::string(input) { validate(); }
void ascii_string::validate() const
{
    for (unsigned char c : *this)
        if (c > 127)
            throw std::invalid_argument("Wrong ascii_string: " + *this);
}

// Преобразование QString в ascii_string с валидацией
inline ascii_string to_ascii_string(const QString & input)
{
    QByteArray byteArray = input.toLatin1();
    ascii_string result(byteArray.constData()); // вызывает валидацию
    return result;
}

// Запись в буфер длины uint16_t и строки
uint16_t fill_buff(const QString & input, std::vector<char> & buffer)
{
    ascii_string ascii = to_ascii_string(input);
    size_t headlength = ascii.length();

    buffer.clear();
    buffer.push_back(static_cast<char>((headlength >> 8) & 0xFF));
    buffer.push_back(static_cast<char>((headlength >> 0) & 0xFF));
    buffer.insert(buffer.end(), ascii.begin(), ascii.end());

    return static_cast<uint16_t>(headlength + 2);
}

// Отправка строки по сокету
void send_header(const QString & header, std::vector<char> & buffer, QTcpSocket *socket)
{
    qDebug() << "Отправка заголовка: " << header;

    uint16_t len = fill_buff(header, buffer);
    qint64 written = socket->write(buffer.data(), len);
    if (written != len)
        throw std::runtime_error("Failed to send full header");
}

// Чтение строки из сокета
void read_header(QString & header, std::vector<char> & buffer, QTcpSocket* socket)
{
    buffer.resize(2);
    qint64 readed = socket->read(buffer.data(), 2);
    if (readed != 2)
        throw std::runtime_error("Failed to read header length");

    uint16_t len = (static_cast<unsigned char>(buffer[0]) << 8) |
                   static_cast<unsigned char>(buffer[1]);

    qDebug() << "NetFunc read_header: ожидаемый размер строки: " << len << " байт";

    if (len == 0)
        throw std::invalid_argument("Empty header");
    if (len > MAX_HEADER_SIZE)
        throw std::invalid_argument("Too long header");

    buffer.resize(len);
    readed = socket->read(buffer.data(), len);
    if (readed != len)
        throw std::runtime_error("Failed to read full header body");

    ascii_string ascii(std::string(buffer.begin(), buffer.end()));
    header = QString::fromUtf8(ascii.c_str(), static_cast<int>(ascii.length()));

    qDebug() << "Получение заголовка: " << header;
}

// Разбиение хэдера "tag:value" на тэг и значение
void parse_header(const QString & header, QString & tag, QString & value)
{
    int pos = header.indexOf(':');
    if (pos == -1)
        throw std::invalid_argument("Header format invalid: " + header.toStdString());

    tag = header.left(pos);
    value = header.mid(pos + 1);
}

quint32 get_file_weigth(std::vector<char> & buffer, QTcpSocket* socket)
{
    buffer.resize(4);
    quint32 readed = socket->read(buffer.data(), 4);
    if(readed != 4)
        throw std::runtime_error("nutduncs: fet_file_len: reading 4 bytes error");

    quint64 file_len = (static_cast<unsigned char>(buffer[0]) << 24 |
                        static_cast<unsigned char>(buffer[1]) << 16 |
                        static_cast<unsigned char>(buffer[2]) << 8 |
                        static_cast<unsigned char>(buffer[3]));

    return file_len;
}

// Загрузка данных из сокета в файл, ограниченная только с верхней границы
quint64 ofstream_write(std::ofstream & filestream, std::vector<char> & buffer, quint64 bytes_to_read, QTcpSocket * socket)
{
    if(!filestream.is_open())
        throw std::runtime_error("netfuncs: dowload file: filestream is closed");

    buffer.resize(bytes_to_read);
    quint64 bytes_readed = socket->read(buffer.data(), bytes_to_read);
    filestream.write(buffer.data(), bytes_readed);

    qDebug() << "netfuncs: dowload file: bytes_readed" << bytes_readed;

    //Возвращаем сколько было действительно прочитано
    return bytes_readed;
}

void build_header(QString & header, const char* tag, const char* value)
{
    header = QString(tag) + ":" + QString(value);
}

void build_header(QString & header, const char* tag, const QString & value)
{
    header = QString(tag) + ":" + value;
}

}

