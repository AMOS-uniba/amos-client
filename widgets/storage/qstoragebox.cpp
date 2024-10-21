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

void QStorageBox::store_sighting(Sighting & sighting, bool del) const {
    if (this->m_enabled) {
        logger.debug(Concern::Storage, QString("Storage \"%1\" storing a sighting").arg(this->id()));

#if SEPARATE_SIGHTINGS
        QString path = QString("%1/%2").arg(this->current_directory().path(), sighting.prefix());
#else
        QString path = this->directory_for_timestamp(sighting.timestamp()).path();
#endif
        del ? sighting.move(path) : sighting.copy(path);
    } else {
        logger.debug(Concern::Storage, QString("Storage \"%1\" disabled, not %2ing").arg(this->id(), del ? "mov" : "copy"));
    }
}

void QStorageBox::discard_sighting(Sighting & sighting) const {
    sighting.discard();
}

QJsonObject QStorageBox::json(void) const {
    QStorageInfo storage_info = this->info();
    return QJsonObject {
        {"on", this->is_enabled()},
        {"a", storage_info.bytesAvailable()},
        {"t", storage_info.bytesTotal()},
    };
}
