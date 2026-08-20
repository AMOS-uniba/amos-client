#include <QTimer>

#include "qscannerbox.h"
#include "../qcamera.h"
#include "utils/exceptions.h"


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

        QString dir = this->m_directory.canonicalPath();
        logger.debug(Concern::Storage, QString("Listing files in %1").arg(dir));

        // One sighting per metadata file, named after it in full. Grouping instead by the first
        // sixteen characters of any file name -- which is a whole UFO name minus the station, but
        // cuts Kvant's event counter off -- collected two Kvant events recorded in the same second
        // into a single Sighting, whose parts then went up under the same form field names and
        // left the server keeping only whichever arrived last.
        const QStringList metadata = this->m_directory.entryList(
            {"*.xml", "*.yaml"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files
        );

        for (const QString & file: metadata) {
            try {
                sightings.append(
                    Sighting(dir, QFileInfo(file).completeBaseName(),
                             (static_cast<QCamera *>(this->parentWidget()))->is_spectral())
                );
            } catch (RuntimeException & e) {
                logger.debug_error(Concern::Sightings, QString("Could not create a sighting: %1").arg(e.what()));
            }
        }

        // Files no metadata file claims are left where they are rather than sent without it,
        // which is also what stops a capture going up twice: once while UFO is still writing its
        // XML, and again once it has.
        int loose = 0;
        for (const QString & file: this->m_directory.entryList(
                 {"*.*"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files)) {
            bool claimed = false;
            for (const Sighting & sighting: sightings) {
                if (file.startsWith(sighting.prefix())) {
                    claimed = true;
                    break;
                }
            }
            if (!claimed) {
                loose += 1;
            }
        }
        if (loose > 0) {
            logger.debug(Concern::Sightings,
                         QString("%1 file(s) in %2 belong to no metadata file").arg(loose).arg(dir));
        }

        if (sightings.count() > 0) {
            logger.debug(Concern::Sightings, QString("%1 sightings found").arg(sightings.count()));
            emit this->sightings_found(sightings);
        }
    } else {
        logger.debug(Concern::Storage, "Scanner disabled, not scanning");
    }
    emit this->sightings_scanned();
}
