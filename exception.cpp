#include "exception.h"

RuntimeException::RuntimeException(const QString& message): std::runtime_error(message.toStdString()) {}

OutOfRange::OutOfRange(const QString& message): RuntimeException(message) {}
