#include "exceptions.h"

RuntimeException::RuntimeException(const QString & message): std::runtime_error(message.toStdString()) {}

MalformedTelegram::MalformedTelegram(const QString & message): RuntimeException(message) {}

EncodingError::EncodingError(const QString & message): RuntimeException(message) {}

ConfigurationError::ConfigurationError(const QString & message): std::runtime_error(message.toStdString()) {}

InvalidState::InvalidState(const QString & message): RuntimeException(message) {}

InvalidSighting::InvalidSighting(const QString & message): RuntimeException(message) {}
