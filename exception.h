#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>

#include "forward.h"

class RuntimeException: public std::runtime_error {
public:
    RuntimeException(const QString& message);
};

class MalformedTelegram: public RuntimeException {
public:
    MalformedTelegram(const QString& message);
};

class EncodingError: public RuntimeException {
public:
    EncodingError(const QString& message);
};

#endif // EXCEPTION_H
