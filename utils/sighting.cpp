#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>

#include "widgets/qcamera.h"
#include "utils/sighting.h"
#include "utils/exceptions.h"
#include "logging/eventlogger.h"

extern EventLogger logger;

Sighting::Sighting(void):
    m_valid(false),
    m_spectral(false),
    m_dir(QDir()),
    m_prefix("")
{}

Sighting::Sighting(const QDir & dir, const QString & prefix, bool spectral):
    m_valid(true),
    m_spectral(spectral),
    m_dir(dir),
    m_prefix(prefix)
{
    this->m_xml  = this->try_open( ".xml",  true);
    this->m_pjpg = this->try_open("P.jpg", false);
    this->m_tjpg = this->try_open("T.jpg", false);
    this->m_mbmp = this->try_open("M.bmp", false);
    this->m_pbmp = this->try_open("P.bmp", false);
    this->m_avi  = this->try_open( ".avi", false);
    this->m_full = QString("%1/%2").arg(this->m_dir.canonicalPath(), this->m_prefix);

    this->m_timestamp = QDateTime::fromString(QFileInfo(this->m_xml).baseName().left(16), "'M'yyyyMMdd_hhmmss");
    this->m_uuid = QUuid::createUuidV5(QUuid{}, this->m_xml);

    logger.debug(
        Concern::Sightings,
        QString("Creating a Sighting from '%1' (%2)")
            .arg(this->m_full, this->spectral_string())
    );

    if (!this->m_timestamp.isValid()) {
        throw RuntimeException("Invalid sighting file name");
    } else {
    }
//    this->hack_Y16(); // Currently disabled, handled by copying script (but it should be solved in UFO)
}

QVector<QString> Sighting::files(void) const {
    return {this->m_xml, this->m_pjpg, this->m_tjpg, this->m_mbmp, this->m_pbmp, this->m_avi};
}

QString Sighting::try_open(const QString & suffix, bool required) {
    QString full_path = QString("%1/%2%3").arg(this->m_dir.canonicalPath(), this->m_prefix, suffix);
    if (QFileInfo::exists(full_path)) {
        return full_path;
    } else {
        if (required) {
            throw InvalidSighting(QString("Could not open sighting file %1").arg(full_path));
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

void Sighting::copy_or_move(const QDir & dir, bool keep) {
    logger.debug(Concern::Sightings, QString("%1ing '%2' to '%3'").arg(keep ? "Copy" : "Mov", this->prefix(), dir.canonicalPath()));
    QDir().mkpath(dir.canonicalPath());

    for (auto & file: this->files()) {
        if (file.isEmpty()) {
            logger.debug(Concern::Sightings, QString("File '%1' not present in the sighting, skipping").arg(file));
        } else {
            QString new_path = QString("%1/%2").arg(dir.canonicalPath(), QFileInfo(file).fileName());

            if (QFile::exists(new_path)) {
                logger.warning(Concern::Sightings,
                               QString("Could not %1 the sighting, deleting file '%2' first...")
                                   .arg(keep ? "copy" : "move", new_path));
                QFile::remove(new_path);
            }

            bool result = keep ? QFile::copy(file, new_path) : QFile::rename(file, new_path);

            if (result) {
                logger.debug(Concern::Sightings, QString("%1ed '%2' to '%3'").arg(keep ? "Copi" : "Mov", file, new_path));
            } else {
                logger.error(Concern::Sightings, QString("Could not %1 file '%2' to '%3'").arg(keep ? "copy" : "move", file, new_path));
            }
            file = new_path;
        }
    }
    this->m_valid &= keep;	// If moving, no longer valid, if copying, keep it
}

void Sighting::move(const QDir & dir) {
    this->copy_or_move(dir, false);
}

void Sighting::copy(const QDir & dir) {
    this->copy_or_move(dir, true);
}

void Sighting::defer(float seconds) {
    this->m_deferred_until = QDateTime::currentDateTimeUtc().addSecs(seconds);
}

void Sighting::discard() {
    logger.warning(Concern::Sightings, QString("Discarding sighting '%1'").arg(this->prefix()));
    try {
        for (auto & file: this->files()) {
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
        logger.error(Concern::Sightings, QString("Error while discarding sighting '%1'").arg(this->prefix()));
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
    for (auto & file: this->files()) {
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
