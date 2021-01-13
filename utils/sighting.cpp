#include "include.h"

extern EventLogger logger;

Sighting::Sighting(const QString& prefix) {
    this->m_jpg = this->try_open(QString("%1P.jpg").arg(prefix));
    this->m_xml = this->try_open(QString("%1.xml").arg(prefix));
    this->m_bmp = this->try_open(QString("%1M.bmp").arg(prefix));
    this->m_avi = this->try_open(QString("%1.avi").arg(prefix));
    this->m_files = {this->m_jpg, this->m_xml, this->m_bmp, this->m_avi};

    logger.info(Concern::Sightings, QString("Created a new sighting '%1*' (%2 MB)").arg(prefix).arg(this->avi_size() / (1 << 20)));
    this->m_timestamp = QFileInfo(this->m_xml).birthTime();
}

Sighting::~Sighting(void) {
    this->m_files.clear();
}

const QString& Sighting::try_open(const QString &path) {
    if (!QFileInfo(path).exists()) {
         throw RuntimeException(QString("Could not open sighting file %1").arg(path));
    }
    return path;
}

qint64 Sighting::avi_size(void) const {
    auto info = QFileInfo(this->m_avi);
    return info.exists() ? info.size() : -1;
}

void Sighting::move(const QString &prefix) {
    logger.debug(Concern::Sightings, QString("Moving to %1").arg(prefix));
    QDir().mkpath(prefix);

    for (auto &file: this->m_files) {
        QString new_path = QString("%1/%2").arg(prefix).arg(QFileInfo(file).fileName());
        QFile::rename(file, new_path);
        file = new_path;
    }
}

void Sighting::copy(const QString &prefix) const {
    logger.debug(Concern::Sightings, QString("Moving to %1").arg(prefix));
    QDir().mkpath(prefix);

    for (auto &file: this->m_files) {
        QString new_path = QString("%1/%2").arg(prefix).arg(QFileInfo(file).fileName());
        QFile::copy(file, new_path);
    }
}

QHttpPart Sighting::jpg_part(void) const {
    QHttpPart jpg_part;
    jpg_part.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
    jpg_part.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QString("form-data; name=\"jpg\"; filename=\"%1\"").arg(QFileInfo(this->m_jpg).fileName())
    );
    QFile jpg_file(this->m_jpg);
    jpg_file.open(QIODevice::ReadOnly);
    jpg_part.setBody(jpg_file.readAll());
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

QHttpPart Sighting::json_metadata(void) const {
    QHttpPart text_part;
    text_part.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    text_part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"meta\"");

    QJsonObject content {
        {"timestamp", QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss.zzz")},
        {"avi_size", this->avi_size() >= 0 ? this->avi_size() : QJsonValue(QJsonValue::Null)},
    };

    text_part.setBody(QJsonDocument(content).toJson(QJsonDocument::Compact));
    return text_part;
}
