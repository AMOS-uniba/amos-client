#include "include.h"

extern EventLogger logger;

Sighting::Sighting(const QString & dir, const QString & prefix):
    m_dir(dir),
    m_prefix(prefix)
{
    QString full = QString("%1/%2").arg(this->m_dir, this->m_prefix);
    this->m_jpg = this->try_open(QString("%1P.jpg").arg(full), false);
    this->m_jpt = this->try_open(QString("%1T.jpg").arg(full), false);
    this->m_xml = this->try_open(QString("%1.xml").arg(full), true);
    this->m_bmp = this->try_open(QString("%1M.bmp").arg(full), false);
    this->m_avi = this->try_open(QString("%1.avi").arg(full), false);
    this->m_files = {this->m_jpg, this->m_jpt, this->m_xml, this->m_bmp, this->m_avi};

    logger.info(Concern::Sightings, QString("Created a new sighting '%1*' (%2 MB) (%3)")
                .arg(prefix)
                .arg(this->avi_size() / (1 << 20))
                .arg(QStringList({
                    (this->m_xml != "") ? "XML" : "---",
                    (this->m_jpg != "") ? "JPG" : "---",
                    (this->m_jpt != "") ? "JPT" : "---",
                    (this->m_bmp != "") ? "BMP" : "---",
                    (this->m_avi != "") ? "AVI" : "---"
                }).join("+"))
    );
    this->m_timestamp = QFileInfo(this->m_xml).birthTime();
//    this->hack_Y16();
}

Sighting::~Sighting(void) {
    this->m_files.clear();
}

QString Sighting::try_open(const QString & path, bool require) {
    if (QFileInfo::exists(path)) {
        return path;
    } else {
        if (require) {
            throw RuntimeException(QString("Could not open sighting file %1").arg(path));
        } else {
            return "";
        }
    }
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

    this->m_jpg = this->m_files[0];
    this->m_jpt = this->m_files[1];
    this->m_xml = this->m_files[2];
    this->m_bmp = this->m_files[3];
    this->m_avi = this->m_files[4];
}

void Sighting::copy(const QString & dir) const {
    logger.debug(Concern::Sightings, QString("Copying to %1").arg(dir));
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

QHttpPart Sighting::jpg_part(void) const {
    QHttpPart jpg_part;
    if (this->m_xml == "") {
        logger.error(Concern::Sightings, QString("XML file not present in sighting %1").arg(this->m_prefix));
    } else {
        jpg_part.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
        jpg_part.setHeader(
            QNetworkRequest::ContentDispositionHeader,
            QString("form-data; name=\"jpg\"; filename=\"%1\"").arg(QFileInfo(this->m_jpg).fileName())
        );
        QFile jpg_file(this->m_jpg);
        jpg_file.open(QIODevice::ReadOnly);
        jpg_part.setBody(jpg_file.readAll());
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
        {"timestamp", QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss.zzz")},
        {"avi_size", this->avi_size() >= 0 ? this->avi_size() : QJsonValue(QJsonValue::Null)},
    };

    auto text = QJsonDocument(content).toJson(QJsonDocument::Compact);
    text_part.setBody(text);
    logger.debug(Concern::Sightings, text);
    return text_part;
}

void Sighting::debug(void) const {
    for (auto & file: this->m_files) {
        qDebug() << file;
    }
}

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
