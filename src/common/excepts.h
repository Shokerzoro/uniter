#ifndef EXCEPTS_H
#define EXCEPTS_H

#include <exception>
#include <stdexcept>
#include <string>

namespace common {
namespace excepts {

    //An exception that should cause the program/thread to terminate, normally
    class unacceptable : public std::runtime_error
    {
    public:
        unacceptable(const std::string & msg) : std::runtime_error(msg) {};
    };

} // namespace exepts
} // namespace common

#endif // EXCEPTS_H
