#include "utils/sighting.h"

#include "exception.h"
#include "utils/exception.h"
#include "logging/eventlogger.h"

#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>


extern EventLogger logger;

Sighting::Sighting(const QString & dir, const QString & prefix, bool spectral):
    m_spectral(spectral),
    m_dir(dir),
    m_prefix(prefix)
{
    QString full = QString("%1/%2").arg(this->m_dir, this->m_prefix);
    this->m_pjpg = this->try_open(QString("%1P.jpg").arg(full), false);
    this->m_tjpg = this->try_open(QString("%1T.jpg").arg(full), false);
    this->m_xml = this->try_open(QString("%1.xml").arg(full), true);
    this->m_mbmp = this->try_open(QString("%1M.bmp").arg(full), false);
    this->m_pbmp = this->try_open(QString("%1P.bmp").arg(full), false);
    this->m_avi = this->try_open(QString("%1.avi").arg(full), false);
    this->m_files = {this->m_pjpg, this->m_tjpg, this->m_xml, this->m_mbmp, this->m_pbmp, this->m_avi};

    this->m_timestamp = QDateTime::fromString(QFileInfo(this->m_xml).baseName().left(16), "'M'yyyyMMdd_hhmmss");
    this->m_uuid = QUuid::createUuidV5(QUuid{}, this->m_xml);

    logger.debug(
        Concern::Sightings,
        QString("Creating a Sighting from '%1' (%2)")
            .arg(full)
            .arg(this->spectral_string())
    );

    if (!this->m_timestamp.isValid()) {
        throw RuntimeException("Invalid sighting file name");
    } else {
    }
//    this->hack_Y16(); // Currently disabled, handled by copying script (but should be solved in UFO)
}

Sighting::~Sighting(void) {
    this->m_files.clear();
}

QString Sighting::try_open(const QString & path, bool required) {
    if (QFileInfo::exists(path)) {
        return path;
    } else {
        if (required) {
            throw RuntimeException(QString("Could not open sighting file %1").arg(path));
        } else {
            return "";
        }
    }
}

QString Sighting::str(void) const {
    return QString("%1 from %2 (%3, %4 MB)")
        .arg(this->spectral_string())
        .arg(this->timestamp().toString("yyyy-MM-dd hh:mm:ss"))
        .arg(QStringList({
            (this->m_xml  != "") ?  "XML" :  "---",
            (this->m_pjpg != "") ? "PJPG" : "----",
            (this->m_tjpg != "") ? "TJPG" : "----",
            (this->m_pbmp != "") ? "PBMP" : "----",
            (this->m_mbmp != "") ? "MBMP" : "----",
            (this->m_avi  != "") ?  "AVI" :  "---"
        }).join("+"))
        .arg(this->avi_size() / (1 << 20));
}

qint64 Sighting::avi_size(void) const {
    auto info = QFileInfo(this->m_avi);
    return info.exists() ? info.size() : -1;
}

void Sighting::move(const QString & dir) {
    QDir().mkpath(dir);

    for (auto & file: this->m_files) {
        if (file.isEmpty()) {
            logger.debug(Concern::Sightings, QString("File not present in the sighting, skipping"));
        } else {
            QString new_path = QString("%1/%2").arg(dir, QFileInfo(file).fileName());

            if (QFile::exists(new_path)) {
                logger.warning(Concern::Sightings, QString("Could not move the sighting, deleting file '%1' first...").arg(new_path));
                QFile::remove(new_path);
            }

            if (QFile::rename(file, new_path)) {
                logger.debug(Concern::Sightings, QString("Moved '%1' to '%2'").arg(file, new_path));
            } else {
                logger.error(Concern::Sightings, QString("Could not move file '%1' to '%2'").arg(file, new_path));
            }

            file = new_path;
        }
    }
}

void Sighting::copy(const QString & dir) const {
    logger.debug(Concern::Sightings, QString("Copying to '%1' to '%2'").arg(this->prefix(), dir));
    QDir().mkpath(dir);

    for (auto & file: this->m_files) {
        QString new_path = QString("%1/%2").arg(dir, QFileInfo(file).fileName());
        if (QFile::copy(file, new_path)) {
            logger.debug(Concern::Sightings, QString("Copied '%1' to '%2'").arg(file, new_path));
        } else {
            logger.error(Concern::Sightings, QString("Could not copy file '%1' to '%2'").arg(file, new_path));
        }
    }
}

void Sighting::discard() {
    logger.warning(Concern::Sightings, QString("Discarding sighting %1").arg(this->prefix()));
    try {
        for (auto & file: this->m_files) {
            if (file.isEmpty()) {
                logger.debug(Concern::Sightings, QString("File not present in the sighting"));
            } else {
                if (QFile::remove(file)) {
                    logger.debug(Concern::Sightings, QString("Deleted file '%1'").arg(file));
                } else {
                    logger.error(Concern::Sightings, QString("Could not delete file '%1'").arg(file));
                }
            }
        }
    } catch (std::exception &) {
        logger.error(Concern::Sightings, QString("Error while removing sighting '%1'").arg(this->prefix()));
    }
}

QHttpPart Sighting::jpg_part(void) const {
    QHttpPart jpg_part;
    if (this->m_xml == "") {
        logger.error(Concern::Sightings, QString("XML file not present in sighting '%1'").arg(this->m_prefix));
    } else {
        jpg_part.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
        jpg_part.setHeader(
            QNetworkRequest::ContentDispositionHeader,
            QString("form-data; name=\"jpg\"; filename=\"%1\"").arg(QFileInfo(this->m_pjpg).fileName())
        );
        QFile pjpg_file(this->m_pjpg);
        pjpg_file.open(QIODevice::ReadOnly);
        jpg_part.setBody(pjpg_file.readAll());
    }
    return jpg_part;
}

QHttpPart Sighting::xml_part(void) const {
    QHttpPart xml_part;
    xml_part.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml; charset=utf-8");
    xml_part.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QString("form-data; name=\"xml\"; filename=\"%1\"").arg(QFileInfo(this->m_xml).fileName())
    );
    QFile xml_file(this->m_xml);
    xml_file.open(QIODevice::ReadOnly);
    xml_part.setBody(xml_file.readAll());
    return xml_part;
}

QHttpPart Sighting::json(void) const {
    QHttpPart text_part;
    text_part.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    text_part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"meta\"");

    QJsonObject content {
        {"spectral", this->is_spectral()},
        {"timestamp", this->m_timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")},
        {"avi_size", this->avi_size() >= 0 ? this->avi_size() : QJsonValue(QJsonValue::Null)},
        {"uuid", this->m_uuid.toString(QUuid::WithBraces)}
    };

    auto text = QJsonDocument(content).toJson(QJsonDocument::Compact);
    text_part.setBody(text);
    logger.debug(Concern::Sightings, QString("Sighting '%1' has content '%2'").arg(this->prefix(), text));
    return text_part;
}

void Sighting::debug(void) const {
    for (auto & file: this->m_files) {
        qDebug() << file;
    }
}

/**
 * @brief Sighting::hack_Y16 tries to fix faulty "Y16" videos by changing the header.
 *        Did not work very well, currently it is disabled and handled by conversion scripts instead
 * @return
 */
bool Sighting::hack_Y16(void) const {
    QFile avi(this->m_avi);

    if (!avi.open(QIODevice::ReadWrite)) {
        return false;
    } else {
        avi.seek(0xBC);

        QByteArray header = avi.read(4);
        if (header == "Y800") {
            logger.debug(Concern::Sightings, "Video format header is correct (Y800)");
        } else {
            if (header == "Y16 ") {
                logger.warning(Concern::Sightings, QString("Video format header is faulty (%1), changing to 'Y800'").arg(QString(header)));
                avi.seek(0xBC);
                avi.write("Y800", 4);
            } else {
                logger.warning(Concern::Sightings, QString("Video format header is unknown (%1), not doing anything").arg(QString(header)));
            }
        }
        avi.close();
        return true;
    }
}
