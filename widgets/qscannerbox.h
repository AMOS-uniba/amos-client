#ifndef QSCANNERBOX_H
#define QSCANNERBOX_H

#include "widgets/qfilesystembox.h"

class QScannerBox: public QFileSystemBox {
    Q_OBJECT
private:

protected:
    virtual QString DialogTitle(void) const override;
    virtual QString AbortMessage(void) const override;

public:
    explicit QScannerBox(QWidget *parent = nullptr);
    void scan_sightings(void) const;

public slots:
    void set_enabled(bool enabled) override;
    void set_directory(const QDir &new_directory) override;

signals:
    void sightings_found(QVector<Sighting> sightings) const;
};

#endif // QSCANNERBOX_H
