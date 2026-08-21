#include <QFileInfo>
#include <QRegularExpression>
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
    m_avi_size(-1),
    m_dir(QDir()),
    m_prefix(""),
    m_counter(0),
    m_status(Status::Unprocessed)
{}


/** Build a sighting from a file list the caller has already decided on.
 *
 *  Ownership is the scanner's job, not this class's: it holds one directory listing and knows every
 *  metadata prefix, which is what it takes to decide that a file belongs to the longest prefix
 *  matching it rather than to every prefix matching it. Claiming files here by `startsWith` meant a
 *  prefix that was a string-prefix of another swallowed its neighbour's files, metadata included,
 *  and two metadata files then went up under the same form field name.
 *
 *  The checks below are the contract that arrangement rests on, kept because the caller could
 *  violate it: a sighting must have files, and it must not carry metadata belonging to someone else.
**/
Sighting::Sighting(const QDir & dir, const QString & prefix, bool spectral, const QStringList & files):
    m_valid(true),
    m_spectral(spectral),
    m_avi_size(-1),
    m_dir(dir),
    m_prefix(prefix),
    m_counter(0),
    m_files(files),
    m_status(Status::Unprocessed)
{
    this->m_full = QString("%1/%2").arg(this->m_dir.canonicalPath(), this->m_prefix);

    if (this->m_files.isEmpty()) {
        throw RuntimeException(QString("No files for sighting '%1'").arg(this->prefix()));
    }

    for (const QString & file: this->m_files) {
        if (Sighting::is_metadata(file) && (QFileInfo(file).completeBaseName() != prefix)) {
            throw RuntimeException(QString("Sighting '%1' was given foreign metadata '%2'")
                                       .arg(prefix, file));
        }
    }

    this->m_avi_size = this->measure_avi();
    // From the prefix, not from a file: the prefix is the name the sighting is known by and covers
    // the whole timestamp in both layouts, so the order of the file list cannot matter here.
    const Sighting::Name name = Sighting::parse_name(prefix);
    this->m_timestamp = name.timestamp;
    this->m_station = name.station;
    this->m_counter = name.counter;
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

bool Sighting::has_metadata(void) const {
    for (const QString & file: this->m_files) {
        if (Sighting::is_metadata(file)) {
            return true;
        }
    }
    return false;
}

/** Take a detector's name apart.
 *  The counter is the interesting half: Kvant emits one because it can record more than one event in
 *  the same second, and discarding it meant two such events could not be told apart downstream. UFO
 *  has none, and reports zero -- which is the truthful answer rather than a missing one, since two
 *  UFO events in one second genuinely are indistinguishable.
 *
 *  Anything that matches neither shape falls back to the old first-fifteen-or-sixteen-characters
 *  reading, so no name that is ingested today stops being ingested.
**/
Sighting::Name Sighting::parse_name(const QString & prefix) {
    static const QRegularExpression ufo(R"(^M(\d{8}_\d{6})_(.*)_$)");
    static const QRegularExpression kvant(R"(^(\d{8}_\d{6})[-_](.+)[-_](\d{5})$)");
    const QString base = QFileInfo(prefix).baseName();

    QRegularExpressionMatch match = ufo.match(prefix);
    if (match.hasMatch()) {
        return {QDateTime::fromString(match.captured(1), "yyyyMMdd_hhmmss"), match.captured(2), 0};
    }

    match = kvant.match(prefix);
    if (match.hasMatch()) {
        return {QDateTime::fromString(match.captured(1), "yyyyMMdd_hhmmss"),
                match.captured(2),
                match.captured(3).toInt()};
    }

    QDateTime timestamp = QDateTime::fromString(base.left(16), "'M'yyyyMMdd_hhmmss");
    if (!timestamp.isValid()) {
        timestamp = QDateTime::fromString(base.left(15), "yyyyMMdd_hhmmss");
    }
    return {timestamp, QString(), 0};
}
/** Suffix tests, both case-insensitive. Windows file systems are, and UFO is not consistent about
 *  the case it writes: comparing case-sensitively meant a ".XML" sighting was still picked up by
 *  the scanner, but then went up with neither its metadata nor its composite -- accepted by the
 *  server and marked stored, with nothing in it.
**/
bool Sighting::has_suffix(const QString & filename, const QString & suffix) {
    return (QFileInfo(filename).suffix().compare(suffix, Qt::CaseInsensitive) == 0);
}

bool Sighting::is_metadata(const QString & filename) {
    return (Sighting::has_suffix(filename, "xml") || Sighting::has_suffix(filename, "yaml"));
}

/** Measure the video, which is not sent but whose size is reported.
 *  Returns -1 when there is none, which is what tells the server not to record a size at all.
**/
qint64 Sighting::measure_avi(void) const {
    for (const QString & filename: this->m_files) {
        if (Sighting::has_suffix(filename, "avi")) {
            return QFileInfo(QString("%1/%2").arg(this->dir_string(), filename)).size();
        }
    }
    return -1;
}

/** Decide whether a file should be sent to the server or not.
 *  The metadata (XML or YAML) and the composite image are sent. The video is not: it is routinely
 *  several gigabytes, and only its size is reported. Neither is the thumbnail -- UFO writes both
 *  "<name>P.jpg" and "<name>T.jpg", and because every part is named after its suffix, sending
 *  both would leave the server keeping whichever of the two happened to arrive last. The bitmaps
 *  ("<name>M.bmp" and "<name>P.bmp") are not sent either.
 *
 *  The composite is recognised by the metadata file beside it: Kvant names it exactly as its
 *  metadata, UFO appends a "P".
**/
bool Sighting::should_send(const QString & path) const {
    if (Sighting::is_metadata(path)) {
        return true;
    }
    if (Sighting::has_suffix(path, "jpg")) {
        // With no metadata to match against, the image is the whole reason this sighting exists
        // and its group holds exactly one, so send it. The metadata follows as its own delivery
        // once the reduction has produced it.
        if (!this->has_metadata()) {
            return true;
        }

        const QString stem = QFileInfo(path).completeBaseName();
        for (const QString & other: this->m_files) {
            if (Sighting::is_metadata(other)) {
                const QString base = QFileInfo(other).completeBaseName();
                if ((stem.compare(base, Qt::CaseInsensitive) == 0) ||
                    (stem.compare(base + "P", Qt::CaseInsensitive) == 0)) {
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
        QString("form-data; name=\"%1\"; filename=\"%2\"").arg(file_info.suffix().toLower(), path)
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
        // Always sent, zero for UFO. A UFO capture has no counter because UFO cannot distinguish two
        // events in one second, so zero for both is the honest report of a real collision; omitting
        // the field would disguise that as "this client does not count".
        {"counter", this->counter()},
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