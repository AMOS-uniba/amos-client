#include "server.h"
#include "mainwindow.h"

Server::Server(MainWindow* _main_window, const QHostAddress& _address, const unsigned short _port, const QString& _station_id):
    main_window(_main_window) {

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
    this->main_window->log_debug(QString("Sending a heartbeat to %1").arg(this->url.toString()));

    QNetworkRequest request(this->url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QJsonDocument document = QJsonDocument(heartbeat);
    QByteArray message = document.toJson(QJsonDocument::Compact);

    this->main_window->log_debug(message);
    QNetworkReply *reply = this->network_manager->post(request, message);

    this->main_window->connect(reply, &QNetworkReply::errorOccurred, this, &Server::heartbeat_error);
}

void Server::heartbeat_error(QNetworkReply::NetworkError error) {
    this->main_window->log_error(QString("Heartbeat could not be sent: error %1").arg(error));
}

void Server::heartbeat_ok(QNetworkReply* reply) {
    this->main_window->log_debug(
        QString("Heartbeat received (HTTP code %1), response \"%2\"")
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString())
                .arg(QString(reply->readAll()))
    );
    reply->deleteLater();
}

void Server::heartbeat_response(void) {
    this->main_window->log_debug("Heartbeat response");
}
