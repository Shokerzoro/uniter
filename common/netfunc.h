#ifndef NETFUNC_H
#define NETFUNC_H

#include <string>
#include <vector>
#include <fstream>
#include <QTcpSocket>
#include <QString>
#include <QFile>

namespace netfuncs {

class ascii_string : public std::string
{
public:
    ascii_string(const std::string & input);
    ascii_string(const char* input);
    ascii_string & operator=(const std::string& input);
    ascii_string & operator=(const char* input);
private:
    void validate(void) const;
};

//Базовые функции
extern quint32 get_file_weigth(std::vector<char> & buffer, QTcpSocket* socket);
//Функции с заголовками
extern void send_header(const QString & header, std::vector<char> & buffer, QTcpSocket* socket);
extern void read_header(QString & header, std::vector<char> & buffer, QTcpSocket* socket);
extern quint64 ofstream_write(std::ofstream & filestream, std::vector<char> & buffer, quint64 bytes_to_read, QTcpSocket * socket);

extern void build_header(QString & header, const char* tag, const char* value);
extern void build_header(QString & header, const char* tag, const QString & value);
extern void parse_header(const QString & header, QString & tag, QString & value);

}



#endif // NETFUNC_H
