#include "TcpConnector.h"
#include "QTcpSocket"
#include <QTimer>
#include <QDataStream>
#include <QDebug>

namespace netfuncs {

TcpConnector::TcpConnector(QObject* parent) : QObject(parent)
{
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &TcpConnector::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &TcpConnector::onDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &TcpConnector::onSockData);


    timer = new QTimer(this);
    timer->setInterval(500); // Период опроса, можно вынести в настройки
    connect(timer, &QTimer::timeout, this, &TcpConnector::onTimerAlert);

    // Инициализация
    sym_read = sym_left = sym_offset = 0;
    bin_read = bin_left = bin_offset = 0;
    ComType = CommunicationType::SYMBOLIC;
    PackState = PacketState::HEAD;
    new_file = nullptr;
}

TcpConnector::~TcpConnector()
{
    if (socket->isOpen()) {
        socket->abort();
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
    if(!new_file || !new_file->isOpen())
        throw std::runtime_error("TcpConnector: invalid file handle!");

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

void TcpConnector::MakeConnect(QString &Ip, quint16 port)
{
    if(socket->state() == QAbstractSocket::UnconnectedState)
    {
        qDebug() << "TcpConnector: попытка подключения к серверу!";
        socket->connectToHost(Ip, port);
        return;
    }
    else if(socket->state() == QAbstractSocket::ConnectingState)
    {
        qDebug() << "TcpConnector: сокет в процессе подключения...";
    }
}

void TcpConnector::MakeDisconnect(void)
{
    if(socket->state() == QTcpSocket::ConnectedState)
        socket->abort();
}

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
    emit signalConnected();
}

void TcpConnector::onDisconnected(void)
{
    qDebug() << "TcpConnector: socket disconnected!";
    emit signalDisconnected();
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

bool TcpConnector::full_cmp(const QString & cmp_tag, const QString & cmp_value)
{ return (this->tag == cmp_tag && this->value == cmp_value); }

bool TcpConnector::tag_cmp(const QString & cmp_tag)
{ return (this->tag == cmp_tag); }

bool TcpConnector::value_cmp(const QString & cmp_value)
{ return (this->value == cmp_value); }

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
    qDebug() << "TcpConnector: sending " + header;
}

void TcpConnector::send_data(QFile *file)
{
    if (!file || !file->isOpen())
        throw std::runtime_error("TcpConnector: invalid file!");

    quint32 size = static_cast<quint32>(file->size());
    QByteArray head;
    head.append(static_cast<char>((size >> 24) & 0xFF));
    head.append(static_cast<char>((size >> 16) & 0xFF));
    head.append(static_cast<char>((size >> 8) & 0xFF));
    head.append(static_cast<char>(size & 0xFF));

    socket->write(head);

    constexpr qint64 chunk = 65536;
    while (!file->atEnd()) {
        socket->write(file->read(chunk));
    }
    socket->flush();

    if (auto_sym_after_bin)
        ComType = CommunicationType::SYMBOLIC;
}


QString TcpConnector::get_tag(void)
{ return tag; }

QString TcpConnector::get_value(void)
{ return value; }

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

// ====== Настройки ======

void TcpConnector::SetAutoSymAfterBin(bool state)
{
    auto_sym_after_bin = state;
}

void TcpConnector::SetAutoBinAfterSym(bool state)
{
    auto_bin_after_sym = state;
}

void TcpConnector::MoveChildenToThread(QThread * NetThread)
{
    socket->moveToThread(NetThread);
    timer->moveToThread(NetThread);
}


} // netfuncs

