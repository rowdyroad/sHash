#pragma once

#include <exception>
extern "C" {
#include <errno.h>
}


namespace NHasher {
namespace NException {
    class Base
        : public std::exception
    {
        public:
            Base(const std::string& what)
                : what_(what)
            {}

            ~Base() throw() {}

           const char* what() const throw()
           {
                return what_.c_str();
           }
        private:
            const std::string what_;

    };

    class Error
        : public Base
    {
        public:
            Error()
                : Base(::strerror(errno))
                , errno_(errno)
            {}

            Error(const std::string& hint)
                : Base(hint + ": " + std::string(::strerror(errno)))
                , errno_(errno)
            {}

            int Errno() const
            {
                return errno_;
            }
        private:
            int errno_;
    };
}
}