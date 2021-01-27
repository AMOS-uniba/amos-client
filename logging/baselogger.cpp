#include "include.h"

BaseLogger::BaseLogger(QObject *parent, const QString &filename):
    QObject(parent),
    m_filename(filename)
{
}

void BaseLogger::initialize(void) {
    this->m_directory.setPath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));

    if (!QDir().mkpath(this->m_directory.path())) {
        throw ConfigurationError(QString("Could not create log folder %1").arg(this->m_directory.path()));
    }

    if (!this->m_filename.isEmpty()) {
        this->m_file = new QFile(QString("%1/%2").arg(this->m_directory.path(), this->m_filename));
        this->m_file->open(QIODevice::Append | QIODevice::Text);
    }
}

QString BaseLogger::filename(void) const {
    return this->m_file->fileName();
}

BaseLogger::~BaseLogger(void) {
    if (this->m_file != nullptr) {
        this->m_file->close();
    }
}
