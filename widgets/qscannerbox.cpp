#include "qscannerbox.h"
#include "utils/exception.h"

extern EventLogger logger;
extern QSettings *settings;

QString QScannerBox::DialogTitle(void) const { return "Select UFO output directory to watch"; }
QString QScannerBox::AbortMessage(void) const { return "Watch directory selection aborted"; }

QScannerBox::QScannerBox(QWidget *parent):
    QFileSystemBox(parent)
{
    this->connect(this->m_timer, &QTimer::timeout, this, &QScannerBox::scan_sightings);
}

void QScannerBox::set_enabled(bool enabled) {
    logger.info(Concern::Storage, QString("Scanner %1abled").arg(enabled ? "en" : "dis"));
    QFileSystemBox::set_enabled(enabled);
    settings->setValue(QString("storage/scanner_enabled"), enabled);
}

void QScannerBox::set_directory(const QDir &new_directory) {
    QFileSystemBox::set_directory(new_directory);
    logger.info(Concern::Storage, QString("Scanner directory set to %1").arg(this->m_directory.path()));
    settings->setValue("storage/scanner_path", this->m_directory.path());
}

void QScannerBox::scan_sightings(void) const {
    if (this->is_enabled()) {
        QVector<Sighting> sightings;

        QString dir = this->m_directory.path();
        logger.debug(Concern::Storage, QString("Listing files in %1").arg(dir));

        QStringList xmls = this->m_directory.entryList({"M*.xml"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);

        for (QString &xml: xmls) {
            try {
                QFileInfo xml_info(QString("%1/%2").arg(dir, xml));
                QString prefix = QString("%1/%2").arg(xml_info.absolutePath(), xml_info.completeBaseName());

                sightings.append(Sighting(prefix));
            } catch (RuntimeException &e) {
                logger.error(Concern::Sightings, QString("Could not create a sighting: %1").arg(e.what()));
            }
        }

        if (sightings.count() > 0) {
            logger.debug(Concern::Sightings, QString("%1 sightings found").arg(sightings.count()));
            emit this->sightings_found(sightings);
        }
    } else {
        logger.debug(Concern::Storage, "Scanner disabled, not scanning");
    }
}
