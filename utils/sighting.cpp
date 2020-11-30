#include "include.h"

extern EventLogger logger;

Sighting::Sighting(const QString &jpg, const QString &xml):
    m_jpg(jpg),
    m_xml(xml) {
    logger.info(QString("Created a new sighting (%1 %2)").arg(jpg).arg(xml));

    QFileInfo info(jpg);
    this->m_timestamp = info.birthTime();
}

QHttpPart Sighting::jpg_part(void) const {
    QHttpPart jpg_part;
    jpg_part.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
    jpg_part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"jpg\"");
    QFile jpg_file(this->m_jpg);
    jpg_file.open(QIODevice::ReadOnly);
    jpg_part.setBodyDevice(&jpg_file);
    return jpg_part;
}

QHttpPart Sighting::xml_part(void) const {
    QHttpPart xml_part;
    xml_part.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml; charset=utf-8");
    xml_part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"xml\"");
    QFile xml_file(this->m_xml);
    xml_file.open(QIODevice::ReadOnly);
    xml_part.setBodyDevice(&xml_file);
    return xml_part;
}

const QString& Sighting::jpg(void) const {
    return this->m_jpg;
}

const QString& Sighting::xml(void) const {
    return this->m_xml;
}
