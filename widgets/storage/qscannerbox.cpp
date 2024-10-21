#include <QTimer>

#include "qscannerbox.h"
#include "../qcamera.h"
#include "utils/exception.h"


extern EventLogger logger;

QString QScannerBox::DialogTitle(void) const { return "Select UFO output directory to watch"; }
QString QScannerBox::AbortMessage(void) const { return "Watch directory selection aborted"; }
QString QScannerBox::MessageEnabled(void) const { return "Scanner \"%1\" %2abled"; }
QString QScannerBox::MessageDirectoryChanged(void) const { return "Scanner \"%1\" set to \"%2\""; }

QScannerBox::QScannerBox(QWidget * parent):
    QFileSystemBox(parent)
{
    this->connect(this->m_timer, &QTimer::timeout, this, &QScannerBox::scan_sightings);
}

void QScannerBox::scan_sightings(void) {
    if (this->is_enabled()) {
        QVector<Sighting> sightings;

        QString dir = this->m_directory.path();
        logger.debug(Concern::Storage, QString("Listing files in %1").arg(dir));

        QStringList xmls = this->m_directory.entryList({"M*.xml"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);

        for (QString & xml: xmls) {
            try {
                QFileInfo xml_info(QString("%1/%2").arg(dir, xml));
                sightings.append(Sighting(xml_info.absolutePath(), xml_info.completeBaseName(),
                                          (static_cast<QCamera *>(this->parentWidget()))->is_spectral()));
            } catch (RuntimeException & e) {
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
