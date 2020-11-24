#include "include.h"

extern EventLogger logger;


Server::Server(const QHostAddress& address, const unsigned short port, const QString& station_id) {
    this->set_url(address, port, station_id);
    this->m_network_manager = new QNetworkAccessManager(this);
    connect(this->m_network_manager, &QNetworkAccessManager::finished, this, &Server::heartbeat_ok);
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

void Server::set_url(const QHostAddress& address, const unsigned short port, const QString& station_id) {
    this->m_address = address;
    this->m_port = port;
    this->m_url = QUrl(QString("http://%1:%2/station/%3/heartbeat/").arg(this->m_address.toString()).arg(this->m_port).arg(station_id));
}

void Server::send_heartbeat(const QJsonObject& heartbeat) const {
    logger.debug(QString("Sending a heartbeat to %1").arg(this->m_url.toString()));

    QNetworkRequest request(this->m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray message = QJsonDocument(heartbeat).toJson(QJsonDocument::Compact);
    logger.debug(QString("Heartbeat assembled: '%1'").arg(QString(message)));

    QNetworkReply *reply = this->m_network_manager->post(request, message);
    this->connect(reply, &QNetworkReply::errorOccurred, this, &Server::heartbeat_error);
}

void Server::heartbeat_error(QNetworkReply::NetworkError error) {
    logger.error(QString("Heartbeat could not be sent: error %1").arg(error));
}

void Server::heartbeat_ok(QNetworkReply* reply) {
    logger.debug(
        QString("Heartbeat received (HTTP code %1), response \"%2\"")
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString())
                .arg(QString(reply->readAll()))
    );
    reply->deleteLater();
}

void Server::heartbeat_response(void) {
    logger.debug("Heartbeat response");
}
