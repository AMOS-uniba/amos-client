#include "exception.h"

RuntimeException::RuntimeException(const QString& message): std::runtime_error(message.toStdString()) {}

MalformedTelegram::MalformedTelegram(const QString& message): RuntimeException(message) {}

EncodingError::EncodingError(const QString& message): RuntimeException(message) {}

ConfigurationError::ConfigurationError(const QString& message): std::runtime_error(message.toStdString()) {}
