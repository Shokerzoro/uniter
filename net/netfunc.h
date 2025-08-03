#ifndef NETFUNC_H
#define NETFUNC_H

#include <string>
#include <vector>
#include <QTcpSocket>
#include <QString>
#include <QFile>

struct TagStrings {
    static constexpr const char* PROTOCOL = "PROTOCOL";
    static constexpr const char* UNETMES = "UNET-MES";
    static constexpr const char* NOUPDATE = "NOUPDATE";
    static constexpr const char* SOMEUPDATE = "SOMEUPDATE";
    static constexpr const char* GETUPDATE = "GETUPDATE";
    static constexpr const char* VERSION = "VERSION";
    static constexpr const char* HASH = "HASH";

    static constexpr const char* NEWDIR = "NEWDIR";
    static constexpr const char* NEWFILE = "NEWFILE";
    static constexpr const char* DELFILE = "DELFILE";
    static constexpr const char* DELDIR = "DELDIR";

    static constexpr const char* AGREE = "AGREE";
    static constexpr const char* REJECT = "REJECT";
    static constexpr const char* COMPLETE = "COMPLETE";
};

class ascii_string
{
public:
    ascii_string(const std::string & input);
    ascii_string(const char* input);
    ascii_string & operator=(const std::string& input);
    ascii_string & operator=(const char* input);
};

//Ошибки выкидываются исключениями, а возврат void
extern void send_header(const QString & header, std::vector<char> & buffer, QTcpSocket* socket);
extern void read_header(QString & header, std::vector<char> & buffer, QTcpSocket* socket);
extern void parse_header(const QString & header, QString & tag, QString & value);
extern void download_file(const QString & FilePath, QTcpSocket* socket);
//Поиск тэга TagStrings::DELFILE или TagStrings::DELDIR внутри
extern void parse_deltag(const QFile * file);
extern QString build_header(const char* tag, const char* value);
extern QString build_header(const char* tag, const QString & value);

#endif // NETFUNC_H
