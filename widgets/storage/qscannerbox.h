#ifndef QSCANNERBOX_H
#define QSCANNERBOX_H

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

public:
    explicit QScannerBox(QWidget * parent = nullptr);
    void scan_sightings(void);

signals:
    void sightings_found(QVector<Sighting> & sightings);
};

#endif // QSCANNERBOX_H
