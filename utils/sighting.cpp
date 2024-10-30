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
    m_prefix(""),
    m_status(Status::Unprocessed)
{}

Sighting::Sighting(const QDir & dir, const QString & prefix, bool spectral):
    m_valid(true),
    m_spectral(spectral),
    m_dir(dir),
    m_prefix(prefix),
    m_status(Status::Unprocessed)
{
    this->m_xml  = this->try_open( ".xml",  true);
    this->m_pjpg = this->try_open("P.jpg", false);
    this->m_tjpg = this->try_open("T.jpg", false);
    this->m_mbmp = this->try_open("M.bmp", false);
    this->m_pbmp = this->try_open("P.bmp", false);
    this->m_avi  = this->try_open( ".avi", false);
    auto info = QFileInfo(this->m_avi);
    this->m_avi_size = info.exists() ? info.size() : 0;

    this->m_full = QString("%1/%2").arg(this->m_dir.canonicalPath(), this->m_prefix);

    this->m_timestamp = QDateTime::fromString(QFileInfo(this->m_xml).baseName().left(16), "'M'yyyyMMdd_hhmmss");
    this->m_deferred_until = QDateTime();
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

QString Sighting::status_string(void) const {
    switch (this->m_status) {
        case Status::Unprocessed:       return "not processed";
        case Status::Deferred:          return QString("deferred until %1").arg(this->deferred_until().toString("hh:mm:ss"));
        case Status::Sent:              return "sent";
        case Status::Duplicate:         return "duplicate";
        case Status::Accepted:          return "accepted";
        case Status::Rejected:          return "rejected";
        case Status::UnknownStation:    return "unknown station";
        case Status::Stored:            return "stored";
        case Status::Discarded:         return "discarded";
    }
    return "";
}

void Sighting::set_status(Status new_status) {
    this->m_status = new_status;
    logger.debug(Concern::Sightings,
                 QString("Status of '%1' set to %2")
                     .arg(this->prefix())
                     .arg(this->status_string()));
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
    this->m_valid = keep;	                // If moving, no longer valid, if copying, keep it
    this->m_dir = keep ? this->m_dir : dir; // If moving, change the directory
    this->set_status(Status::Stored);
}

void Sighting::move(const QDir & dir) {
    this->copy_or_move(dir, false);
}

void Sighting::copy(const QDir & dir) {
    this->copy_or_move(dir, true);
}

void Sighting::defer(float seconds) {
    logger.debug(Concern::Sightings, QString("Deferring sighting '%1'").arg(this->prefix()));
    this->m_deferred_until = QDateTime::currentDateTimeUtc().addSecs(seconds);
}

double Sighting::deferred_for(void) const {
    if (this->deferred_until().isValid()) {
        return static_cast<double>((this->deferred_until() - QDateTime::currentDateTimeUtc()).count()) / 1000.0;
    } else {
        return std::numeric_limits<double>::quiet_NaN();
    }
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
        this->set_status(Status::Discarded);
        this->m_deferred_until = QDateTime();
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
