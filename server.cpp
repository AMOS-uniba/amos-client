#include "server.h"

Server::Server(const QHostAddress& _address, const unsigned short _port):
    address(_address),
    port(_port) {
}

void Server::set_url(const QHostAddress& address, const unsigned short port, const QString& station_id) {
    this->address = address;
    this->port = port;
    this->url = QUrl(QString("http://%1:%2/station/%3/heartbeat/").arg(this->address.toString()).arg(this->port).arg(station_id));
}

void Server::send_heartbeat(void) const {
    this->log_debug("About to send a heartbeat to " + address);

    QNetworkRequest request(this->url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QJsonDocument document = QJsonDocument(this->station->prepare_heartbeat());
    QByteArray message = document.toJson(QJsonDocument::Compact);

    this->log_debug(message);
    QNetworkReply *reply = this->network_manager->post(request, message);

    connect(reply, &QNetworkReply::errorOccurred, this, &MainWindow::heartbeat_error);
}
