#include "include.h"

extern EventLogger logger;


Server::Server(const QHostAddress& address, const unsigned short port, const QString& station_id) {
    this->set_url(address, port, station_id);
    this->m_network_manager = new QNetworkAccessManager(this);
    this->connect(this->m_network_manager, &QNetworkAccessManager::finished, this, &Server::heartbeat_ok);
}

Server::~Server() {
    delete this->m_network_manager;
}

QHostAddress Server::address(void) const {
    return this->m_address;
}

unsigned short Server::port(void) const {
    return this->m_port;
}

void Server::set_url(const QHostAddress &address, const unsigned short port, const QString &station_id) {
    this->m_address = address;
    this->m_port = port;
    this->m_url_heartbeat = QUrl(
        QString("http://%1:%2/station/%3/heartbeat/")
            .arg(this->m_address.toString())
            .arg(this->m_port)
            .arg(station_id)
    );
    this->m_url_sighting = QUrl(
        QString("http://%1:%2/station/%3/sighting/")
            .arg(this->m_address.toString())
            .arg(this->m_port)
            .arg(station_id)
    );
    QString full_address = QString("%1:%2").arg(this->m_address.toString()).arg(this->m_port);
    logger.info(Concern::Server, QString("Address set to %1").arg(full_address));

    emit this->url_set(full_address);
}

void Server::send_heartbeat(const QJsonObject &heartbeat) const {
    logger.debug(Concern::Server, QString("Sending a heartbeat to %1").arg(this->m_url_heartbeat.toString()));

    QNetworkRequest request(this->m_url_heartbeat);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray message = QJsonDocument(heartbeat).toJson(QJsonDocument::Compact);
    logger.debug(Concern::Server, QString("Heartbeat assembled: '%1'").arg(QString(message)));

    QNetworkReply *reply = this->m_network_manager->post(request, message);
    this->connect(reply, &QNetworkReply::errorOccurred, this, &Server::heartbeat_error);
}

void Server::heartbeat_error(QNetworkReply::NetworkError error) {
    auto reply = static_cast<QNetworkReply*>(sender());
    logger.error(Concern::Server,
                 QString("Heartbeat could not be sent: %1 (error %2: %3)")
                    .arg(QString(reply->readAll()))
                    .arg(error)
                    .arg(reply->errorString())
    );
}

void Server::heartbeat_ok(QNetworkReply* reply) {
    logger.debug(
        Concern::Server,
        QString("Heartbeat received (HTTP code %1), response \"%2\"")
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString())
                .arg(QString(reply->readAll()))
    );
    reply->deleteLater();

    emit this->heartbeat_sent();
}

void Server::send_sighting(const Sighting &sighting) const {
    logger.debug(Concern::Server, QString("Sending a sighting to %1").arg(this->m_url_sighting.toString()));

    QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    multipart->append(sighting.jpg_part());
    multipart->append(sighting.xml_part());
    multipart->append(sighting.json_metadata());

    QNetworkRequest request(this->m_url_sighting);
    QNetworkReply *reply = this->m_network_manager->post(request, multipart);
    multipart->setParent(reply); // delete the multiPart with the reply */
}
