#include <QDateTime>
#include <QTextStream>

#include "logging/baselogger.h"
#include "utils/exceptions.h"

BaseLogger::BaseLogger(QObject * parent, const QString & filename):
    QObject(parent),
    m_filename(filename)
{}

BaseLogger::~BaseLogger(void) {
    if (this->m_file != nullptr) {
        this->m_file->close();
        delete this->m_file;
    }
}

void BaseLogger::initialize(void) {
    this->m_directory.setPath(".");

    if (!QDir().mkpath(this->m_directory.path())) {
        throw ConfigurationError(QString("Could not create log folder %1").arg(this->m_directory.path()));
    }

    if (!this->m_filename.isEmpty()) {
        this->m_file = new QFile(QString("%1/%2").arg(this->m_directory.path(), this->m_filename));
        this->m_file->open(QIODevice::Append | QIODevice::Text);
    }
}

void BaseLogger::rotate_if_oversized(void) const {
    if ((this->m_file == nullptr) || (this->m_file->size() < BaseLogger::MaxSize)) {
        return;
    }

    const QString base = this->m_file->fileName();
    this->m_file->close();

    /* Drop the oldest first and then shift each one down a place, so every rename lands on a name
     * that has just been vacated -- QFile::rename refuses to overwrite, and doing it in the other
     * order would either fail or lose a file still worth keeping.
     */
    QFile::remove(QString("%1.%2").arg(base).arg(BaseLogger::Generations));
    for (int generation = BaseLogger::Generations - 1; generation >= 1; --generation) {
        QFile::rename(QString("%1.%2").arg(base).arg(generation),
                      QString("%1.%2").arg(base).arg(generation + 1));
    }
    QFile::rename(base, QString("%1.1").arg(base));

    if (this->m_file->open(QIODevice::Append | QIODevice::Text)) {
        /* Say so in the fresh file. Without this a log simply begins mid-session, which reads like
         * lost data rather than a rollover.
         */
        QTextStream out(this->m_file);
        out << QString("---- rolled over at %1, the previous file is now %2.1 ----")
                   .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate), this->m_filename)
            << Qt::endl;
    }
}

QString BaseLogger::filename(void) const {
    return this->m_file->fileName();
}
