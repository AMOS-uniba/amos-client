#include "include.h"

extern EventLogger logger;

Storage::Storage(const QString &name, const QDir &directory): m_name(name), m_root_directory(directory) {
    this->update_active_directory();
}

QStorageInfo Storage::info(void) const {
    return QStorageInfo(this->m_root_directory);
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
    return this->m_root_directory;
}

void Storage::set_directory(const QDir &dir) {
    logger.info(QString("Storage '%1' set to %2").arg(this->m_name).arg(dir.path()));
    this->m_root_directory = dir;
}

const QString& Storage::get_name(void) const {
    return this->m_name;
}

void Storage::update_active_directory(void) {
    QDateTime now = QDateTime::currentDateTimeUtc();

    this->m_active_directory = QDir(QString("%1/%2/%3/%4")
        .arg(this->m_root_directory.path())
        .arg(now.toString("yyyy"))
        .arg(now.toString("yyyyMM"))
        .arg(now.toString("yyyyMMdd"))
    );
}

QVector<Sighting> Storage::list_new_sightings(void) {
    QVector<Sighting> sightings;

    QString dir = this->m_root_directory.path();
    logger.info(QString("Listing files in %1").arg(dir));

    QStringList xmls = this->m_root_directory.entryList({"*.xml"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);

    for (QString xml: xmls) {
        QString xml_path = dir + "/" + xml;
        QFileInfo xml_info(xml_path);
        QString jpg_path = dir + "/" + xml_info.completeBaseName() + "P.jpg";
        QFileInfo jpg_info(jpg_path);

        logger.info(QString("Found file '%1'").arg(xml_path));
        if (QFile::exists(jpg_path) && jpg_info.isFile()) {
            logger.info(QString("Also found JPG file '%1'").arg(jpg_path));
            sightings.append(Sighting(jpg_path, xml_path));
        } else {
            logger.info(QString("Could not find corresponding JPG '%1'").arg(jpg_path));
        }
    }

    this->update_active_directory();
    return sightings;
}
