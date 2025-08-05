#ifndef EXCEPTS_H
#define EXCEPTS_H

#include <exception>
#include <stdexcept>
#include <string>

namespace excepts {

    //Исключение, которое должно приводить к завершению программы / потока, как правиль
    class unacceptable : public std::runtime_error
    {
    public:
        unacceptable(const std::string & msg) : std::runtime_error(msg) {};
    };

}

#endif // EXCEPTS_H
