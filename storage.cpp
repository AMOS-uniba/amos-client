#include "include.h"

extern EventLogger logger;

Storage::Storage(const QString &name, const QDir &directory): m_name(name), m_root_directory(directory) {
}

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

QVector<Sighting> Storage::list_new_sightings(void) {
    QVector<Sighting> sightings;

    QString dir = this->m_root_directory.path();
    logger.debug(QString("Listing files in %1").arg(dir));

    QStringList xmls = this->m_root_directory.entryList({"M*.xml"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);

    for (QString xml: xmls) {
        try {
            QFileInfo xml_info(QString("%1/%2").arg(dir).arg(xml));
            QString prefix = QString("%1/%2").arg(xml_info.absolutePath()).arg(xml_info.completeBaseName());

            sightings.append(Sighting(prefix));
        } catch (RuntimeException &e) {
            logger.error(QString("Could not create a sighting: %1").arg(e.what()));
        }

    }
    return sightings;
}

void Storage::move_sighting(Sighting &sighting) {
    logger.debug("Moving a sighting...");
    sighting.move(QString("%1/%2/")
        .arg(this->m_root_directory.path())
        .arg(QDateTime::currentDateTimeUtc().toString("yyyy/MM/dd"))
    );
}
