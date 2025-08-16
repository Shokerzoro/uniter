#include "TcpConnector.h"
#include <QTimer>
#include <QDataStream>
#include <QDebug>

namespace netfuncs {

TcpConnector::TcpConnector(QTcpSocket* socket, QObject* parent) : QObject(parent), socket(socket)
{
    connect(socket, &QTcpSocket::connected, this, &TcpConnector::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &TcpConnector::onSockData);

    timer = new QTimer(this);
    timer->setInterval(500); // Период опроса, можно вынести в настройки
    connect(timer, &QTimer::timeout, this, &TcpConnector::onTimerAlert);

    // Инициализация
    sym_read = sym_left = sym_offset = 0;
    bin_read = bin_left = bin_offset = 0;
}

TcpConnector::~TcpConnector()
{
    if (socket) {
        socket->disconnect(this);
    }
}

// ====== Переопределяемые функции

void TcpConnector::process_communication(void)
{
    switch (ComType) {
    case CommunicationType::SYMBOLIC:
    {
        if(PackState == PacketState::HEAD)
        {
            this->process_sym_head();
        }
        if(PackState == PacketState::BODY)
        {
            if(this->process_sym_body())
                emit signalSymReady();
        }
        break;
    }
    case CommunicationType::BINARY:
    {
        if(PackState == PacketState::HEAD)
        {
            this->process_bin_head();
        }
        if(PackState == PacketState::BODY)
        {
            if(this->process_bin_body())
                emit signalBinReady();
        }
        break;
    }
    default:
        break;
    }

    this->purge_buffer();
}

bool TcpConnector::process_sym_head(void)
{
    if(symbolic_buffer.size() - sym_offset >= sym_head_size)
    {
        quint64 sym_body_length = 0;
        for(int counter = 0; counter < sym_head_size; counter++)
        {
            sym_body_length = (sym_body_length << 8) | static_cast<unsigned char>(symbolic_buffer[sym_offset]);
            sym_offset++;
        }

        qDebug() << "TcpConnector: ожидаемая длина строки: " << sym_body_length;

        if(sym_body_length > max_symbody_size)
            throw std::invalid_argument("TcpConnector: too long header");

        sym_read = 0;
        sym_left = sym_body_length;
        PackState = PacketState::BODY;
        return true;
    }
    return false;
}

bool TcpConnector::process_sym_body(void)
{
    //Дописывает во временный буфер
    quint16 bytes_to_read = qMin(symbolic_buffer.size() - sym_offset, sym_left);
    sym_offset += bytes_to_read;
    sym_read += bytes_to_read;
    sym_left -= bytes_to_read;

    if(sym_left == 0)
    {
        QByteArray body = symbolic_buffer.mid(sym_offset - sym_read, sym_read);
        sym_offset += sym_read;
        header = QString::fromUtf8(body);

        int pos = header.indexOf(':');
        tag = header.left(pos);
        value = header.mid(pos + 1);

        if(!ascii_validate(header))
            throw std::invalid_argument("TcpConnector: invalid header format");

        qDebug() << "TcpConnector: got header: " << header;

        if(auto_bin_after_sym)
            ComType = CommunicationType::BINARY;
        PackState = PacketState::HEAD;

        return true;
    }
    return false;
}

bool TcpConnector::process_bin_head(void)
{
    if(binary_buffer.size() - bin_offset >= bin_head_size)
    {
        quint64 bin_body_length = 0;
        for(int counter = 0; counter < bin_head_size; counter++)
        {
            bin_body_length = (bin_body_length << 8) | static_cast<unsigned char>(binary_buffer[bin_offset]);
            bin_offset++;
        }

        qDebug() << "TcpConnector: ожидаемый размер файла: " << bin_body_length;

        bin_read = 0;
        bin_left = bin_body_length;
        PackState = PacketState::BODY;
        return true;
    }
    return false;
}

bool TcpConnector::process_bin_body(void)
{
    if(!new_file->isOpen())
        throw std::runtime_error("TcpConnector: file closed!");

    quint32 this_time = qMin(binary_buffer.size() - bin_offset, bin_left);
    if(new_file->write(binary_buffer.constData() + bin_offset, this_time) != this_time)
        throw std::runtime_error("TcpConnector: write error!");
    bin_offset += this_time;
    bin_read += this_time;
    bin_left -= this_time;

    if(bin_left == 0)
    {
        if(auto_sym_after_bin)
            ComType = CommunicationType::SYMBOLIC;

        PackState = PacketState::HEAD;
        return true;
    }
    return false;
}

// ====== Слоты ======

void TcpConnector::onConnected(void)
{
    qDebug() << "TcpConnector: socket connected!";
    ComType = CommunicationType::SYMBOLIC;
    PackState = PacketState::HEAD;
    // Инициализация
    symbolic_buffer.clear();
    sym_read = sym_left = sym_offset = 0;
    binary_buffer.clear();
    bin_read = bin_left = bin_offset = 0;
}

void TcpConnector::onSockData(void)
{
    qDebug() << "TcpConnector: got data on socket!";
    switch (ComType) {
    case CommunicationType::SYMBOLIC:
        symbolic_buffer.append(socket->readAll());
        this->process_communication();
        break;
    case CommunicationType::BINARY:
        binary_buffer.append(socket->readAll());
        this->process_communication();
        break;
    default:
        break;
    }
}

void TcpConnector::onTimerAlert(void)
{
    this->process_communication();
}

// ====== Методы изменения типа взаимодействия ======

void TcpConnector::process_symbolic(void)
{
    ComType = CommunicationType::SYMBOLIC;
    PackState = PacketState::HEAD;
    symbolic_buffer.clear();
    sym_read = sym_left = sym_offset = 0;
}

void TcpConnector::process_binary(QFile* new_file)
{
    Q_ASSERT(new_file);
    this->new_file = new_file;
    ComType = CommunicationType::BINARY;
    PackState = PacketState::HEAD;
    binary_buffer.clear();
    bin_read = bin_left = bin_offset = 0;
}

// ====== Основной публичный интерфейс ======

bool TcpConnector::full_cmp(const QString &tag, const QString &value)
{ return (this->tag == tag && this->value == value); }

bool TcpConnector::tag_cmp(const QString &tag)
{ return (this->tag == tag); }

bool TcpConnector::value_cmp(const QString &value)
{ return (this->value == value); }

void TcpConnector::reply(const QString & tag, const QString & value)
{
    header = tag + ':' + value;
    if(!ascii_validate(header) || header.size() > max_symbody_size)
    {
        qWarning() << "TcpConnector: non ascii or too long header:" << header;
        return;
    }

    QByteArray temp_header_buffer; // Используем временный буфер
    quint16 header_length = static_cast<quint16>(header.size());
    temp_header_buffer.append(static_cast<char>((header_length >> 8) & 0xFF));
    temp_header_buffer.append(static_cast<char>(header_length & 0xFF));
    temp_header_buffer.append(header.toUtf8());

    socket->write(temp_header_buffer);
}

// ====== Вспомогательные методы ======

void TcpConnector::purge_buffer(void)
{
    switch (ComType) {
    case CommunicationType::SYMBOLIC:
    {
        if(sym_offset > max_sym_buffer)
        {
            symbolic_buffer.remove(0, sym_offset);
            sym_offset = 0;
        }
        break;
    }
    case CommunicationType::BINARY:
    {
        if(bin_offset > max_bin_buffer)
        {
            binary_buffer.remove(0, bin_offset);
            bin_offset = 0;
        }
        break;
    }
    default:
        break;
    }
}

bool TcpConnector::ascii_validate(const QString &s)
{
    for (QChar c : s) {
        if (c.unicode() > 127) return false;
    }
    return true;
}

} // netfuncs

