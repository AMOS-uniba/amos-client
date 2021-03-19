#include "include.h"
#include "qserver.h"
#include "ui_qserver.h"

extern EventLogger logger;
extern QSettings *settings;


QServer::QServer(QWidget *parent):
    QConfigurable(parent),
    ui(new Ui::QServer)
{
    this->ui->setupUi(this);

    this->m_network_manager = new QNetworkAccessManager(this);
    this->connect(this->m_network_manager, &QNetworkAccessManager::finished, this, &QServer::heartbeat_ok);

    this->connect(this->ui->le_station_id, &QLineEdit::textChanged, this, &QServer::handle_settings_changed);
    this->connect(this->ui->le_ip, &QLineEdit::textChanged, this, &QServer::handle_settings_changed);
    this->connect(this->ui->sb_port, QOverload<int>::of(&QSpinBox::valueChanged), this, &QServer::handle_settings_changed);
    this->connect(this->ui->bt_apply, &QPushButton::clicked, this, &QServer::apply_settings);
    this->connect(this->ui->bt_discard, &QPushButton::clicked, this, &QServer::discard_settings);

    this->connect(this->ui->bt_send_heartbeat, &QPushButton::clicked, this, &QServer::button_send_heartbeat);

    this->connect(this, &QServer::settings_changed, this, &QServer::refresh_urls);
    this->connect(this, &QServer::settings_changed, this, &QServer::discard_settings);
    this->connect(this, &QServer::settings_changed, this, &QServer::save_settings);

}

QServer::~QServer() {
    delete this->ui;
    delete this->m_network_manager;
}

void QServer::initialize(Station * const station) {
    this->m_station = station;
    this->load_settings(settings);
}

void QServer::load_settings_inner(const QSettings * const settings) {
    this->set_station_id(
        settings->value("station/id", "none").toString()
    );
    this->set_address(
        settings->value("server/ip", "127.0.0.1").toString(),
        settings->value("server/port", 4805).toInt()
    );
    this->refresh_urls();
}

void QServer::save_settings(void) {
    settings->setValue("station/id", this->station_id());
    settings->setValue("server/ip", this->address().toString());
    settings->setValue("server/port", this->port());
    settings->sync();
}

const QHostAddress& QServer::address(void) const { return this->m_address; }
const unsigned short& QServer::port(void) const { return this->m_port; }
const QString& QServer::station_id(void) const { return this->m_station_id; }

void QServer::set_address(const QString &address, const unsigned short port) {
    QHostAddress addr;
    if (!addr.setAddress(address)) {
        throw ConfigurationError(QString("Invalid IP address %1").arg(address));
    }

    this->m_address = addr;
    this->m_port = port;
    QString full_address = QString("%1:%2").arg(this->m_address.toString()).arg(this->m_port);
    logger.info(Concern::Server, QString("Address set to %1").arg(full_address));
}

void QServer::set_station_id(const QString &id) {
    if (id.length() > 4) {
        throw ConfigurationError(QString("Cannot set station id to '%1'").arg(id));
    }

    this->m_station_id = id;
    logger.info(Concern::Configuration, QString("Station id set to '%1'").arg(this->m_station_id));
}

void QServer::refresh_urls(void) {
    this->m_url_heartbeat = QUrl(
        QString("http://%1:%2/station/%3/heartbeat/")
            .arg(this->m_address.toString())
            .arg(this->m_port)
            .arg(this->m_station_id)
    );
    this->m_url_sighting = QUrl(
        QString("http://%1:%2/station/%3/sighting/")
            .arg(this->m_address.toString())
            .arg(this->m_port)
            .arg(this->m_station_id)
    );
}

void QServer::send_heartbeat(const QJsonObject &heartbeat) const {
    logger.debug(Concern::Server, QString("Sending a heartbeat to %1").arg(this->m_url_heartbeat.toString()));

    QNetworkRequest request(this->m_url_heartbeat);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray message = QJsonDocument(heartbeat).toJson(QJsonDocument::Compact);
    logger.debug(Concern::Server, QString("Heartbeat assembled: '%1'").arg(QString(message)));

    QNetworkReply *reply = this->m_network_manager->post(request, message);
    this->connect(reply, &QNetworkReply::errorOccurred, this, &QServer::heartbeat_error);
}

void QServer::heartbeat_error(QNetworkReply::NetworkError error) {
    auto reply = static_cast<QNetworkReply*>(sender());
    logger.error(
        Concern::Server,
        QString("Heartbeat could not be sent: %1 (error %2: %3)")
                .arg(QString(reply->readAll()))
                .arg(error)
                .arg(reply->errorString()
        )
    );
}

void QServer::heartbeat_ok(QNetworkReply* reply) {
    logger.debug(
        Concern::Server,
        QString("Heartbeat received (HTTP code %1), response \"%2\"").arg(
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString(),
            QString(reply->readAll())
        )
    );
    reply->deleteLater();

    emit this->heartbeat_sent();
}

void QServer::send_sighting(const Sighting &sighting) const {
    logger.debug(Concern::Server, QString("Sending a sighting to %1").arg(this->m_url_sighting.toString()));

    QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    multipart->append(sighting.jpg_part());
    multipart->append(sighting.xml_part());
    multipart->append(sighting.json_metadata());

    QNetworkRequest request(this->m_url_sighting);
    QNetworkReply *reply = this->m_network_manager->post(request, multipart);
    multipart->setParent(reply); // delete the multiPart with the reply */
}

bool QServer::is_changed(void) {
    return (
        (this->ui->le_station_id->text() != this->station_id()) ||
        (this->ui->le_ip->text() != this->address().toString()) ||
        (this->ui->sb_port->value() != this->port())
    );
}

void QServer::apply_settings_inner(void) {
    if (this->ui->le_station_id->text() != this->station_id()) {
        this->set_station_id(this->ui->le_station_id->text());
    }

    if ((this->ui->le_ip->text() != this->address().toString()) || (this->ui->sb_port->value() != this->port())) {
        this->set_address(this->ui->le_ip->text(), this->ui->sb_port->value());
    }
}

void QServer::discard_settings(void) {
    this->ui->le_ip->setText(this->address().toString());
    this->ui->sb_port->setValue(this->port());
    this->ui->le_station_id->setText(this->station_id());
}

void QServer::button_send_heartbeat(void) {
    logger.info(Concern::Server, "Sending a heartbeat (manual)");
    this->send_heartbeat(this->m_station->prepare_heartbeat());
}
