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
    m_avi_size(-1),
    m_dir(dir),
    m_prefix(prefix),
    m_status(Status::Unprocessed)
{
    this->m_full = QString("%1/%2").arg(this->m_dir.canonicalPath(), this->m_prefix);

    for (const auto & entry: QDirListing(dir.canonicalPath(),
                                         QDirListing::IteratorFlag::FilesOnly)) {
        const QString name = entry.fileName();
        if (entry.fileName().startsWith(prefix) && name.contains('.')) {
            this->m_files.append(entry.fileName());
        }
    }

    if (this->m_files.isEmpty()) {
        throw RuntimeException(QString("No files for sighting '%1'").arg(this->prefix()));
    }

    this->m_avi_size = this->measure_avi();
    this->m_timestamp = this->parse_timestamp(*this->m_files.begin());
    this->m_deferred_until = QDateTime();

    logger.debug(
        Concern::Sightings,
        QString("Creating a Sighting '%1' from '%2' (%3), %4 files")
            .arg(this->m_timestamp.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(this->m_full, this->spectral_string())
            .arg(this->m_files.length())
    );

    if (!this->m_timestamp.isValid()) {
        throw RuntimeException(QString("Invalid sighting timestamp '%1' for prefix '%2', ignoring")
            .arg(this->m_timestamp.toString("yyyy-MM-dd"))
            .arg(prefix)
        );
    }
}
QDateTime Sighting::parse_timestamp(const QString & path) {
    const QString base = QFileInfo(path).baseName();

    QDateTime ts = QDateTime::fromString(base.left(16), "'M'yyyyMMdd_hhmmss");
    // UFO output
    if (ts.isValid()) {
        return ts;
    } else {
        // Kvant output
        return QDateTime::fromString(base.left(15), "yyyyMMdd_hhmmss");
    }
}
/** Measure the video, which is not sent but whose size is reported.
 *  Returns -1 when there is none, which is what tells the server not to record a size at all.
**/
qint64 Sighting::measure_avi(void) const {
    for (const QString & filename: this->m_files) {
        if (filename.endsWith(".avi", Qt::CaseInsensitive)) {
            return QFileInfo(QString("%1/%2").arg(this->dir_string(), filename)).size();
        }
    }
    return -1;
}

/** Decide whether a file should be sent to the server or not.
 *  The metadata (XML or YAML) and the composite image are sent. The video is not: it is routinely
 *  several gigabytes, and only its size is reported. Neither is the thumbnail -- UFO writes both
 *  "<name>P.jpg" and "<name>T.jpg", and because every part is named after its suffix, sending
 *  both would leave the server keeping whichever of the two happened to arrive last.
 *
 *  The composite is recognised by the metadata file beside it rather than by the sighting prefix,
 *  which the scanner truncates to sixteen characters and which therefore matches no file in full.
**/
bool Sighting::should_send(const QString & path) const {
    if (path.endsWith(".xml") || path.endsWith(".yaml")) {
        return true;
    }
    if (path.endsWith(".jpg")) {
        const QString stem = QFileInfo(path).completeBaseName();
        for (const QString & other: this->m_files) {
            if (other.endsWith(".xml") || other.endsWith(".yaml")) {
                // Kvant names the composite exactly as its metadata, UFO appends a "P"
                const QString base = QFileInfo(other).completeBaseName();
                if ((stem == base) || (stem == base + "P")) {
                    return true;
                }
            }
        }
    }
    return false;
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
        .arg(this->spectral_string(),
             this->timestamp().toString("yyyy-MM-dd hh:mm:ss"),
             QStringList(this->m_files.cbegin(), this->m_files.cend()).join(" + "))
        .arg(this->avi_size() / (1 << 20));
}

QString Sighting::status_string(void) const {
    switch (this->m_status) {
        case Status::Unprocessed:       return "not processed";
        case Status::Sent:              return "sent";
        case Status::Timeout:           return "timeout";
        case Status::Duplicate:         return "duplicate";
        case Status::Accepted:          return "accepted";
        case Status::Rejected:          return "rejected";
        case Status::Stored:            return "stored";
        case Status::RemoteHostClosed:  return "host not found";
        case Status::UnknownError:      return "unknown error";
        case Status::Quarantined:       return "quarantined";
    }
    return "<error>";
}

void Sighting::set_status(Status new_status) {
    this->m_status = new_status;
    logger.debug(Concern::Sightings,
                 QString("Status of '%1' set to %2").arg(
                         this->prefix(),
                         this->status_string()));
}

bool Sighting::move(const QDir & dir) {
    QDir().mkpath(dir.path());
    logger.debug(Concern::Sightings, QString("Moving '%1' to '%2'").arg(this->prefix(), dir.canonicalPath()));

    bool success = true;

    for (auto & filename: this->m_files) {
        if (filename.isEmpty()) {
            logger.debug(Concern::Sightings, QString("File '%1' not present in the sighting, skipping").arg(filename));
        } else {
            QFile file(this->dir().canonicalPath() + "/" + filename);
            QString new_path = QString("%1/%2").arg(dir.canonicalPath(), QFileInfo(filename).fileName());

            if (QFile::exists(new_path)) {
                logger.warning(Concern::Sightings,
                               QString("Could not move the sighting, deleting file '%3' first...")
                                   .arg(new_path));
                QFile::remove(new_path);
            }

            bool result = file.rename(new_path);
            success &= result;

            if (result) {
                logger.debug(Concern::Sightings, QString("Moved '%1' to '%2'").arg(filename, new_path));
            } else {
                logger.error(Concern::Sightings,
                             QString("Could not move file '%1' to '%2': '%3' (%4)")
                                .arg(filename, new_path, file.errorString())
                                .arg(file.error()));
            }
            filename = new_path;
        }
    }
    if (success) {
        this->m_valid = false;
        this->m_dir = dir;
        this->undefer();
    } else {
        logger.error(Concern::Sightings, QString("Error when moving sighting '%1'").arg(this->prefix()));
    }
    return success;
}

void Sighting::defer(float seconds) {
    logger.debug(Concern::Sightings, QString("Deferring sighting '%1'").arg(this->prefix()));
    this->m_deferred_until = QDateTime::currentDateTimeUtc().addSecs(seconds);
}

void Sighting::undefer(void) {
    this->m_deferred_until = QDateTime();
}

double Sighting::deferred_for(void) const {
    if (this->deferred_until().isValid()) {
        return static_cast<double>((this->deferred_until() - QDateTime::currentDateTimeUtc()).count()) / 1000.0;
    } else {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

QList<QHttpPart> Sighting::assemble(void) const {
    QList<QHttpPart> out = {};
    out.append(this->json());
    for (const auto & file: this->m_files) {
        if (this->should_send(file)) {
            out.append(this->build_part(file));
        }
    }
    return out;
}

QHttpPart Sighting::build_part(const QString & filename) const {
    QString path = QString("%1/%2").arg(this->dir_string(), filename);
    QFileInfo file_info(path);

    QHttpPart part;
    part.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/octet-stream"
    );
    part.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QString("form-data; name=\"%1\"; filename=\"%2\"").arg(file_info.suffix(), path)
    );

    QFile part_file(path);
    if (part_file.open(QIODevice::ReadOnly)) {
        part.setBody(part_file.readAll());
    } else {
        logger.warning(Concern::Sightings, QString("Could not open file '%1'").arg(path));
    }
    return part;
}

QHttpPart Sighting::json(void) const {
    QHttpPart text_part;
    text_part.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    text_part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"meta\"");

    QJsonObject content {
        {"spectral", this->is_spectral()},
        {"timestamp", this->m_timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")},
        {"avi_size", this->avi_size() >= 0 ? this->avi_size() : QJsonValue(QJsonValue::Null)},
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