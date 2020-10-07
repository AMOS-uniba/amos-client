#include "exception.h"

RuntimeException::RuntimeException(const QString& message): std::runtime_error(message.toStdString()) {

}
