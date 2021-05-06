#include "qstoragebox.h"

extern EventLogger logger;
extern QSettings * settings;

QString QStorageBox::DialogTitle(void) const { return "Select storage directory"; }
QString QStorageBox::AbortMessage(void) const { return "Storage directory selection aborted"; }

QStorageBox::QStorageBox(QWidget *parent):
    QFileSystemBox(parent),
    m_name("")
{}

const QString& QStorageBox::name(void) const {
    return this->m_name;
}

void QStorageBox::set_name(const QString &name) {
    if (this->m_name.isEmpty()) {
        this->m_name = name;
    }
}

const QDir QStorageBox::current_directory(const QDateTime &datetime) const {
    return QDir(QString("%1/%2/").arg(this->m_directory.path(), datetime.toString("yyyy/MM/dd")));
}

void QStorageBox::set_enabled(bool enabled) {
    logger.info(Concern::Storage, QString("Storage \"%1\" %2abled").arg(this->name(), enabled ? "en" : "dis"));
    QFileSystemBox::set_enabled(enabled);

    settings->setValue(QString("storage/%1_enabled").arg(this->name()), enabled);
}

void QStorageBox::set_directory(const QDir &new_directory) {
    QFileSystemBox::set_directory(new_directory);
    logger.info(Concern::Storage, QString("Storage \"%1\": directory set to %2").arg(this->m_name, this->m_directory.path()));

    settings->setValue(QString("storage/%1_path").arg(this->name()), this->m_directory.path());
}

void QStorageBox::store_sighting(Sighting &sighting, bool del) const {
    if (this->m_enabled) {
        logger.debug(Concern::Storage, QString("Storage %1 enabled").arg(this->m_name));
        if (del) {
            sighting.move(this->current_directory().path());
        } else {
            sighting.copy(this->current_directory().path());
        }
    } else {
        logger.debug(Concern::Storage, QString("Storage %1 disabled, not %2ing").arg(this->m_name, del ? "mov" : "copy"));
    }
}

QJsonObject QStorageBox::json(void) const {
    QStorageInfo storage_info = this->info();
    return QJsonObject {
        {"a", storage_info.bytesAvailable()},
        {"t", storage_info.bytesTotal()},
    };
}
