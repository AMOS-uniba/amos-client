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

        QStringList files = this->m_directory.entryList({"*.*"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);

        QSet<QString> prefixes;
        for (const QString & file: files) {
            QFileInfo file_info(QString("%1/%2").arg(dir, file));
            prefixes.insert(file_info.completeBaseName().left(16));
        }

        for (const QString & prefix: prefixes) {
            try {
                sightings.append(
                    Sighting(dir, prefix, (static_cast<QCamera *>(this->parentWidget()))->is_spectral())
                );
            } catch (RuntimeException & e) {
                logger.debug_error(Concern::Sightings, QString("Could not create a sighting: %1").arg(e.what()));
            }
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
