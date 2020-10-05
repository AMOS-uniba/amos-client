#include "include.h"

extern Log logger;

Storage::Storage(const QString& _name, const QDir& _directory): name(_name), directory(_directory) {}

QStorageInfo Storage::info(void) const {
    return QStorageInfo(this->directory);
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
    return this->directory;
}

void Storage::set_directory(const QDir& dir) {
    logger.info(QString("Storage '%1' set to %2").arg(this->name).arg(dir.path()));
    this->directory = dir;
}

const QString& Storage::get_name(void) const {
    return this->name;
}
