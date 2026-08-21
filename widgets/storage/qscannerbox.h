#ifndef QSCANNERBOX_H
#define QSCANNERBOX_H

#include <QSet>

#include "widgets/storage/qfilesystembox.h"

/**
 * @brief The QScannerBox class scans the specified directory for new files,
 *        and if anything is found, it emits a `sightings_found` signal.
 */
class QScannerBox: public QFileSystemBox {
    Q_OBJECT
protected:
    virtual QString DialogTitle(void) const override;
    virtual QString AbortMessage(void) const override;
    virtual QString MessageEnabled(void) const override;
    virtual QString MessageDirectoryChanged(void) const override;

    /** How long every file under one prefix must sit untouched before the capture counts as
     *  complete. UFO writes the metadata, the composite, the thumbnail, the bitmaps and the video
     *  in no fixed order, and a two-second scan lands in the middle of that often enough.
    **/
    constexpr static qint64 SettleTime = 10000;

    /** How long a composite image waits for metadata that may never come. The reduction that
     *  produces it can run hours after the capture, so past this the image is delivered on its own
     *  and the metadata follows later as a separate delivery; the server merges the two by
     *  timestamp. Long enough that a detector writing its own files seconds apart is unaffected.
    **/
    constexpr static qint64 MetadataGrace = 600000;

    // Prefixes already announced as metadata-less, so that the notice is logged once rather than
    // on every scan for as long as the image sits there.
    QSet<QString> m_announced;

public:
    explicit QScannerBox(QWidget * parent = nullptr);
    void scan_sightings(void);

signals:
    void sightings_scanned(void);
    void sightings_found(QVector<Sighting> & sightings);
};

#endif // QSCANNERBOX_H
