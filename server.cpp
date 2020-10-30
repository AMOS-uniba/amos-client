#include "include.h"

extern Log logger;

Server::Server(const QHostAddress& _address, const unsigned short _port, const QString& _station_id) {
    this->set_url(_address, _port, _station_id);
    this->network_manager = new QNetworkAccessManager(this);
    connect(this->network_manager, &QNetworkAccessManager::finished, this, &Server::heartbeat_ok);
}

Server::~Server() {
    delete this->network_manager;
}

QHostAddress Server::get_address(void) const {
    return this->address;
}

unsigned short Server::get_port(void) const {
    return this->port;
}

void Server::set_url(const QHostAddress& address, const unsigned short port, const QString& station_id) {
    this->address = address;
    this->port = port;
    this->url = QUrl(QString("http://%1:%2/station/%3/heartbeat/").arg(this->address.toString()).arg(this->port).arg(station_id));
}

void Server::send_heartbeat(const QJsonObject& heartbeat) const {
    logger.debug(QString("Sending a heartbeat to %1").arg(this->url.toString()));

    QNetworkRequest request(this->url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray message = QJsonDocument(heartbeat).toJson(QJsonDocument::Compact);
    logger.debug(QString("Heartbeat assembled: '%1'").arg(QString(message)));

    QNetworkReply *reply = this->network_manager->post(request, message);
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
