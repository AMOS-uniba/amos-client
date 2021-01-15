#include "include.h"

extern EventLogger logger;

Storage::Storage(const QString &name, const QDir &directory): m_name(name), m_root_directory(directory) {}

QStorageInfo Storage::info(void) const {
    return QStorageInfo(this->m_root_directory);
}

// return JSON info for heartbeat composition
QJsonObject Storage::json(void) const {
    QStorageInfo storage_info = this->info();
    return QJsonObject {
        {"a", storage_info.bytesAvailable()},
        {"t", storage_info.bytesTotal()},
    };
}

const QDir Storage::current_directory(void) const {
    return QString("%1/%2/").arg(this->m_root_directory.path()).arg(QDateTime::currentDateTimeUtc().toString("yyyy/MM/dd"));
}

const QDir& Storage::root_directory(void) const {
    return this->m_root_directory;
}

void Storage::set_root_directory(const QDir &dir) {
    logger.info(Concern::Storage, QString("Storage '%1' set to %2").arg(this->m_name).arg(dir.path()));
    this->m_root_directory = dir;
}

const QString& Storage::name(void) const {
    return this->m_name;
}

void Storage::store_sighting(Sighting &sighting, bool del) const {
    if (del) {
        sighting.move(this->current_directory().path());
    } else {
        sighting.copy(this->current_directory().path());
    }
}
