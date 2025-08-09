#ifndef NETFUNC_H
#define NETFUNC_H

#include <string>
#include <vector>
#include <QTcpSocket>
#include <QString>
#include <QFile>

namespace netfuncs {

/*
class NetConnector
{
public:
    NetConnector(QTcpSocket* socket);
    //Для отправки

    //Для получения

    //Для сравнения

    //Для настройки

private:
    QTcpSocket* m_socket;
    QString m_header, m_tag, m_value;
};
*/

class ascii_string
{
public:
    ascii_string(const std::string & input);
    ascii_string(const char* input);
    ascii_string & operator=(const std::string& input);
    ascii_string & operator=(const char* input);
private:
    void validate(void) const;
};

//Ошибки выкидываются исключениями, а возврат void
extern void send_header(const QString & header, std::vector<char> & buffer, QTcpSocket* socket);
extern void read_header(QString & header, std::vector<char> & buffer, QTcpSocket* socket);
extern void parse_header(const QString & header, QString & tag, QString & value);
extern void download_file(const QString & FilePath, QTcpSocket* socket);

extern QString build_header(const char* tag, const char* value);
extern QString build_header(const char* tag, const QString & value);

}



#endif // NETFUNC_H
