#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>
#include <QString>

class RuntimeException: public std::runtime_error {
public:
    RuntimeException(const QString & message);
};

class MalformedTelegram: public RuntimeException {
public:
    MalformedTelegram(const QString & message);
};

class EncodingError: public RuntimeException {
public:
    EncodingError(const QString & message);
};

class ConfigurationError: public std::runtime_error {
public:
    ConfigurationError(const QString & message);
};

class InvalidState: public RuntimeException {
public:
    InvalidState(const QString & message);
};

#endif // EXCEPTION_H
