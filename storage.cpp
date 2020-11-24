#include "include.h"

extern EventLogger logger;

Storage::Storage(const QString& _name, const QDir& _directory): m_name(_name), m_directory(_directory) {}

QStorageInfo Storage::info(void) const {
    return QStorageInfo(this->m_directory);
}

// return JSON info for heartbeat composition
QJsonObject Storage::json(void) const {
    QStorageInfo storage_info = this->info();
    return QJsonObject {
        {"a", (double) storage_info.bytesAvailable() / (1024 * 1024 * 1024)},
        {"t", (double) storage_info.bytesTotal() / (1024 * 1024 * 1024)},
    };
}

const QDir& Storage::get_directory(void) const {
    return this->m_directory;
}

void Storage::set_directory(const QDir& dir) {
    logger.info(QString("Storage '%1' set to %2").arg(this->m_name).arg(dir.path()));
    this->m_directory = dir;
}

const QString& Storage::get_name(void) const {
    return this->m_name;
}
