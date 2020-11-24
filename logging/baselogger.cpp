#include "baselogger.h"

BaseLogger::BaseLogger(QObject *parent, const QString &filename): QObject(parent) {
    if (!filename.isEmpty()) {
        this->m_file = new QFile();
        this->m_file->setFileName(filename);
        this->m_file->open(QIODevice::Append | QIODevice::Text);
    }
}

BaseLogger::~BaseLogger(void) {
    if (this->m_file != nullptr) {
        this->m_file->close();
    }
}
