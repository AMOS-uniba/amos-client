#include <QJsonObject>
#include "qstoragebox.h"

extern EventLogger logger;
extern QSettings * settings;

QString QStorageBox::DialogTitle(void) const { return "Select storage directory"; }
QString QStorageBox::AbortMessage(void) const { return "Storage directory selection aborted"; }
QString QStorageBox::MessageEnabled(void) const { return "Storage \"%1\" %2abled"; }
QString QStorageBox::MessageDirectoryChanged(void) const { return "Storage \"%1\" directory set to \"%2\""; }

QStorageBox::QStorageBox(QWidget * parent):
    QFileSystemBox(parent)
{}

const QDir QStorageBox::directory_for_timestamp(const QDateTime & datetime) const {
    return QDir(QString("%1/%2/").arg(this->m_directory.path(), datetime.toString("yyyy/MM/dd")));
}

const QDir QStorageBox::quarantine_directory(const Sighting & sighting) const {
    return QDir(QString("%1/%2/%3/").arg(this->m_directory.path(), "quarantine", sighting.prefix()));
}

void QStorageBox::store_sighting(Sighting & sighting) const {
    logger.debug(Concern::Storage, QString("Storage \"%1\" storing a sighting").arg(this->id()));
    QString path = this->directory_for_timestamp(sighting.timestamp()).path();
    sighting.move(path);
}

void QStorageBox::quarantine_sighting(Sighting & sighting) const {
    logger.debug(Concern::Storage, QString("Storage \"%1\" quarantining a sighting").arg(this->id()));
    QString path = this->quarantine_directory(sighting).path();
    QDir().mkdir(path);
    sighting.move(path);
}

QJsonObject QStorageBox::json(void) const {
    QStorageInfo storage_info = this->info();
    return QJsonObject {
        {"on", this->is_enabled()},
        {"a", storage_info.bytesAvailable()},
        {"t", storage_info.bytesTotal()},
    };
}
