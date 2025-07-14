#ifndef QSTORAGEBOX_H
#define QSTORAGEBOX_H

#include "widgets/storage/qfilesystembox.h"
#include "utils/sighting.h"

class QStorageBox: public QFileSystemBox {
    Q_OBJECT
protected:
    virtual QString DialogTitle(void) const override;
    virtual QString AbortMessage(void) const override;
    virtual QString MessageEnabled(void) const override;
    virtual QString MessageDirectoryChanged(void) const override;

public:
    explicit QStorageBox(QWidget * parent = nullptr);
    QJsonObject json(void) const;
    const QDir directory_for_timestamp(const QDateTime & datetime = QDateTime::currentDateTimeUtc()) const;

public slots:
    void store_sighting(Sighting & sighting) const;
    void discard_sighting(Sighting & sighting) const;

signals:
    void sighting_stored(const QString & name);
};

#endif // QSTORAGEBOX_H
