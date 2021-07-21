#ifndef QSCANNERBOX_H
#define QSCANNERBOX_H

#include "widgets/storage/qfilesystembox.h"
#include "widgets/qcamera.h"

class QScannerBox: public QFileSystemBox {
    Q_OBJECT
protected:
    virtual QString DialogTitle(void) const override;
    virtual QString AbortMessage(void) const override;
    virtual QString MessageEnabled(void) const override;
    virtual QString MessageDirectoryChanged(void) const override;

public:
    explicit QScannerBox(QWidget * parent = nullptr);
    void scan_sightings(void) const;

signals:
    void sightings_found(QVector<Sighting> sightings) const;
};

#endif // QSCANNERBOX_H
