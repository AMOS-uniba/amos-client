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

        logger.debug(Concern::Storage, QString("Listing files in %1").arg(this->m_directory.canonicalPath()));

        // One listing serves every pass below: which files are metadata, how long ago each file was
        // written to, which prefix owns each file, and which files nothing owns at all.
        const QFileInfoList entries = this->m_directory.entryInfoList(
            {"*.*"}, QDir::Filter::NoDotAndDotDot | QDir::Filter::Files
        );

        // One sighting per metadata file, named after it in full. Grouping instead by the first
        // sixteen characters of any file name -- which is a whole UFO name minus the station, but
        // cuts Kvant's event counter off -- collected two Kvant events recorded in the same second
        // into a single Sighting, whose parts then went up under the same form field names and
        // left the server keeping only whichever arrived last.
        //
        // Not deduplicated on purpose: an "X.xml" beside an "X.yaml" yields the prefix X twice and
        // emits the same sighting twice, which the model collapses. That sighting carries both
        // files, which is fine -- their form field names do not collide.
        QStringList prefixes;
        for (const QFileInfo & entry: entries) {
            if (Sighting::is_metadata(entry.fileName())) {
                prefixes.append(entry.completeBaseName());
            }
        }

        const QDateTime now = QDateTime::currentDateTimeUtc();
        QStringList settled;
        int unsettled = 0;

        for (const QString & prefix: prefixes) {
            /* Wait until nothing under this prefix has been written to for SettleTime. A Sighting
             * freezes the file list it is given, so building one mid-capture delivers whatever half
             * of the set happened to exist and never revisits the rest.
             *
             * This test is deliberately broad -- every entry matching the prefix, not just the ones
             * the prefix ends up owning. A sibling's metadata file may not have been written yet, in
             * which case its files legitimately match this shorter prefix and nothing distinguishes
             * them; settling on the broad set is the only thing that stops this prefix delivering
             * while the neighbour is still being written.
             */
            qint64 idle = QScannerBox::SettleTime;
            for (const QFileInfo & entry: entries) {
                if (entry.fileName().startsWith(prefix, Qt::CaseInsensitive)) {
                    idle = qMin(idle, entry.lastModified().msecsTo(now));
                }
            }
            if (idle < QScannerBox::SettleTime) {
                logger.debug(Concern::Sightings,
                             QString("Sighting '%1' was written to %2 ms ago, waiting for it to settle")
                                 .arg(prefix).arg(idle));
                unsettled += 1;
            } else {
                settled.append(prefix);
            }
        }

        if (unsettled > 0) {
            logger.debug(Concern::Sightings,
                         QString("%1 sighting(s) in %2 are still being written to")
                             .arg(unsettled).arg(this->m_directory.canonicalPath()));
        }

        /* Decide which prefix owns each file: a metadata file always owns itself, anything else goes
         * to the longest prefix matching it. Longest-prefix-wins alone is not enough -- with "X.xml"
         * beside "X.x.xml", "X.xml" starts with the longer prefix "X.x" and would be handed to it,
         * leaving the prefix X with no files at all.
         */
        QMap<QString, QStringList> owned;
        QFileInfoList unowned;
        for (const QFileInfo & entry: entries) {
            const QString name = entry.fileName();
            QString best;

            if (Sighting::is_metadata(name)) {
                best = entry.completeBaseName();
            } else {
                for (const QString & prefix: prefixes) {
                    if (name.startsWith(prefix, Qt::CaseInsensitive) && (prefix.length() > best.length())) {
                        best = prefix;
                    }
                }
            }

            if (best.isEmpty()) {
                unowned.append(entry);
            } else {
                owned[best].append(name);
            }
        }

        for (const QString & prefix: settled) {
            try {
                sightings.append(Sighting(this->m_directory, prefix,
                                          (static_cast<QCamera *>(this->parentWidget()))->is_spectral(),
                                          owned.value(prefix)));
            } catch (RuntimeException & e) {
                logger.debug_error(Concern::Sightings, QString("Could not create a sighting: %1").arg(e.what()));
            }
        }

        /* A capture whose metadata never arrived. The reduction that produces it may run hours after
         * the event, so once the image has waited out MetadataGrace it is delivered on its own and
         * the metadata follows later as its own delivery, which the server merges onto the same row
         * by timestamp.
         *
         * Keyed on the image's own name, NOT on the prefix the metadata will eventually have: were
         * they the same, the later metadata would look like a sighting the model has already
         * delivered, be ignored, and never be sent at all.
         */
        int loose = 0;
        for (const QFileInfo & entry: unowned) {
            const qint64 age = entry.lastModified().msecsTo(now);

            if (Sighting::has_suffix(entry.fileName(), "jpg") && (age >= QScannerBox::MetadataGrace)) {
                const QString prefix = entry.completeBaseName();
                if (!this->m_announced.contains(prefix)) {
                    this->m_announced.insert(prefix);
                    logger.info(Concern::Sightings,
                                QString("No metadata for '%1' after %2 s, sending the image alone")
                                    .arg(prefix).arg(age / 1000));
                }
                try {
                    sightings.append(Sighting(this->m_directory, prefix,
                                              (static_cast<QCamera *>(this->parentWidget()))->is_spectral(),
                                              {entry.fileName()}));
                } catch (RuntimeException & e) {
                    logger.debug_error(Concern::Sightings, QString("Could not create a sighting: %1").arg(e.what()));
                }
            } else {
                loose += 1;
            }
        }

        if (loose > 0) {
            logger.debug(Concern::Sightings,
                         QString("%1 file(s) in %2 belong to no metadata file")
                             .arg(loose).arg(this->m_directory.canonicalPath()));
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
