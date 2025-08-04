#ifndef UNACCAPTABLE_H
#define UNACCAPTABLE_H

#include <exception>
#include <stdexcept>
#include <string>

namespace exc {

    //Исключение, которое должно приводить к завершению программы / потока, как правиль
    class unacceptable : public std::runtime_error
    {
    public:
        unacceptable(const std::string & msg) : std::runtime_error(msg) {};
    };

}

#endif // UNACCAPTABLE_H
