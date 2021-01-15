#include "include.h"

extern EventLogger logger;

FileSystemScanner::FileSystemScanner(const QDir &directory):
    m_directory(directory)
{
    this->m_timer = new QTimer(this);
    this->m_timer->setInterval(1000);
    this->connect(this->m_timer, &QTimer::timeout, this, &FileSystemScanner::scan);
    this->m_timer->start();
}


void FileSystemScanner::scan(void) const {
    QVector<Sighting> sightings;

    QString dir = this->m_directory.path();
    logger.debug(Concern::Storage, QString("Listing files in %1").arg(dir));

    QStringList xmls = this->m_directory.entryList({"M*.xml"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);

    for (QString xml: xmls) {
        try {
            QFileInfo xml_info(QString("%1/%2").arg(dir).arg(xml));
            QString prefix = QString("%1/%2").arg(xml_info.absolutePath()).arg(xml_info.completeBaseName());

            sightings.append(Sighting(prefix));
        } catch (RuntimeException &e) {
            logger.error(Concern::Sightings, QString("Could not create a sighting: %1").arg(e.what()));
        }
    }

    if (sightings.count() > 0) {
        logger.debug(Concern::Sightings, QString("%1 sightings found").arg(sightings.count()));
        emit this->sightings_found(sightings);
    }
}

QDir FileSystemScanner::directory(void) const { return this->m_directory; }
