#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>

#include "forward.h"

class RuntimeException: public std::runtime_error {
public:
    RuntimeException(const QString& message);
};


class OutOfRange: public RuntimeException {
public:
    OutOfRange(const QString& message);
};

#endif // EXCEPTION_H
