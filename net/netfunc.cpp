#include "../exeptions/unacceptable.h"

#include <QTcpSocket>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QtEndian>
#include <QDebug>
#include <vector>
#include <stdexcept>
#include <string>

#define MAX_HEADER_SIZE 255

// Внутренний класс для валидации ASCII-строки
class ascii_string : public std::string
{
public:
    ascii_string(const std::string & input) : std::string(input)
    { validate(); }
    ascii_string(const char* input) : std::string(input)
    { validate(); }

private:
    void validate() const
    {
        for (unsigned char c : *this)
            if (c > 127)
                throw std::invalid_argument("Wrong ascii_string: " + *this);
    }
};

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

    if (headlength > MAX_HEADER_SIZE - 6)
        throw std::invalid_argument("Too big msg to header");

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

// Заглушка: Скачивание и сохранение файла
void download_file(const QString & filepath, QTcpSocket *socket)
{
    qDebug() << "Начинаем загрузку файла: " << filepath;
    //Читаем 4 байта и переворачиваем их
    QByteArray sizeBytes = socket->read(4);
    if (sizeBytes.size() < 4)
        throw std::runtime_error("NetFunc download_file: недостаточно данных для размера файла");
    quint32 fileSize = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sizeBytes.constData()));

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly))
        throw std::runtime_error("NetFunc download_file: не удалось открыть файл для записи");

    quint32 bytesReceived = 0;
    while (bytesReceived < fileSize) {
        if (!socket->waitForReadyRead(5000))
            throw std::runtime_error("NetFunc download_file: таймаут ожидания данных файла");

        QByteArray chunk = socket->read(fileSize - bytesReceived);
        if (chunk.isEmpty())
            throw std::runtime_error("NetFunc download_file: ошибка чтения из сокета");

        file.write(chunk);
        bytesReceived += chunk.size();
    }

    file.close();
    qDebug() << "Файл загружен: " << filepath;
}

QString build_header(const char* tag, const char* value)
{
    return QString(tag) + ":" + QString(value);
}

QString build_header(const char* tag, const QString & value)
{
    return QString(tag) + ":" + value;
}

