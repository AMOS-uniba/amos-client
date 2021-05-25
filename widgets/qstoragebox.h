#ifndef QSTORAGEBOX_H
#define QSTORAGEBOX_H

#include <QDir>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QGridLayout>
#include <QFileDialog>

#include <QStorageInfo>

#include "widgets/qfilesystembox.h"
#include "logging/eventlogger.h"
#include "utils/sighting.h"


class QStorageBox: public QFileSystemBox {
    Q_OBJECT
private:
    QString m_name;
protected:
    virtual QString DialogTitle(void) const override;
    virtual QString AbortMessage(void) const override;
public:
    explicit QStorageBox(QWidget * parent = nullptr);

    const QString & name(void) const;
    void set_name(const QString & name);

    QJsonObject json(void) const;
    const QDir current_directory(const QDateTime & datetime = QDateTime::currentDateTimeUtc()) const;

public slots:
    void set_enabled(bool enabled) override;
    void set_directory(const QDir & new_directory) override;

    void store_sighting(Sighting & sighting, bool del = false) const;

signals:
    void sighting_stored(const QString & name);
};

#endif // QSTORAGEBOX_H
