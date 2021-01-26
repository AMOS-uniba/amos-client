#include "include.h"

extern EventLogger logger;

Storage::Storage(const QString &name, const QDir &directory):
    FileSystemManager(directory),
    m_name(name)
{}

// JSON info for heartbeat composition
QJsonObject Storage::json(void) const {
    QStorageInfo storage_info = this->info();
    return QJsonObject {
        {"a", storage_info.bytesAvailable()},
        {"t", storage_info.bytesTotal()},
    };
}

const QDir Storage::current_directory(const QDateTime &datetime) const {
    return QDir(QString("%1/%2/").arg(this->m_directory.path()).arg(datetime.toString("yyyy/MM/dd")));
}

void Storage::set_directory(const QDir &directory) {
    logger.info(Concern::Storage, QString("Root directory of storage '%1' set to %2").arg(this->m_name).arg(directory.path()));
    FileSystemManager::set_directory(directory);

    emit this->directory_set(directory);
}

const QString& Storage::name(void) const {
    return this->m_name;
}

void Storage::store_sighting(Sighting &sighting, bool del) const {
    if (this->m_enabled) {
        if (del) {
            sighting.move(this->current_directory().path());
        } else {
            sighting.copy(this->current_directory().path());
        }
    } else {
        logger.debug(Concern::Storage, QString("Storage %1 disabled, not %2ing").arg(this->m_name).arg(del ? "mov" : "copy"));
    }
}

void Storage::set_enabled(bool enabled) {
    logger.info(Concern::Storage, QString("Storage %1 is now %2abled").arg(this->m_name).arg(enabled ? "en" : "dis"));

    if (enabled != this->m_enabled) {
        this->m_enabled = enabled;
        emit this->toggled(this->m_enabled);
    }
}
