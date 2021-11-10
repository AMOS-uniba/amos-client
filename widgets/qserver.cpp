#include "widgets/qstation.h"

#include "widgets/qserver.h"
#include "ui_qserver.h"

extern EventLogger logger;
extern QSettings * settings;


QServer::QServer(QWidget * parent):
    QAmosWidget(parent),
    ui(new Ui::QServer)
{
    this->ui->setupUi(this);

    this->m_network_manager = new QNetworkAccessManager(this);
    this->connect(this->m_network_manager, &QNetworkAccessManager::finished, this, &QServer::heartbeat_ok);

    this->connect(this->ui->bt_send_heartbeat, &QPushButton::clicked, this, &QServer::button_send_heartbeat);

    this->connect(this, &QServer::settings_saved, this, &QServer::refresh_urls);
}

QServer::~QServer() {
    delete this->ui;
    delete this->m_network_manager;
}

void QServer::initialize(QSettings * settings) {
    QAmosWidget::initialize(settings);
    this->refresh_urls();
}

void QServer::connect_slots(void) {
    this->connect(this->ui->le_station_id, &QLineEdit::textChanged, this, &QServer::settings_changed);
    this->connect(this->ui->le_ip, &QLineEdit::textChanged, this, &QServer::settings_changed);
    this->connect(this->ui->sb_port, QOverload<int>::of(&QSpinBox::valueChanged), this, &QServer::settings_changed);
}

void QServer::load_settings_inner(void) {
    this->set_station_id(
        this->m_settings->value("station/id", "none").toString()
    );
    this->set_address(
        this->m_settings->value("server/ip", "127.0.0.1").toString(),
        this->m_settings->value("server/port", 4805).toInt()
    );
    this->refresh_urls();
}

void QServer::load_defaults(void) {
    this->set_station_id("none");
    this->set_address("127.0.0.1", 4805);
}

void QServer::save_settings_inner(void) const {
    this->m_settings->setValue("station/id", this->station_id());
    this->m_settings->setValue("server/ip", this->address().toString());
    this->m_settings->setValue("server/port", this->port());
}

void QServer::set_address(const QString & address, const unsigned short port) {
    QHostAddress addr;
    if (!addr.setAddress(address)) {
        throw ConfigurationError(QString("Invalid address \"%1\"").arg(address));
    }

    this->m_address = addr;
    this->m_port = port;
    this->refresh_urls();

    QString full_address = QString("%1:%2").arg(this->m_address.toString()).arg(this->m_port);
    logger.info(Concern::Server, QString("Address set to %1").arg(full_address));
}

void QServer::set_station_id(const QString & id) {
    if ((id.length() < 2) || (id.length() > 4)) {
        throw ConfigurationError(QString("Cannot set station id to '%1'").arg(id));
    }

    this->m_station_id = id;
    this->refresh_urls();

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

void QServer::send_heartbeat(const QJsonObject & heartbeat) const {
    logger.debug(Concern::Server, QString("Sending a heartbeat to %1").arg(this->m_url_heartbeat.toString()));

    QNetworkRequest request(this->m_url_heartbeat);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray message = QJsonDocument(heartbeat).toJson(QJsonDocument::Compact);
    logger.debug(Concern::Server, QString("Heartbeat assembled: '%1'").arg(QString(message)));

    QNetworkReply * reply = this->m_network_manager->post(request, message);
    this->connect(reply, &QNetworkReply::errorOccurred, this, &QServer::heartbeat_error);
    this->m_last_heartbeat = QDateTime::currentDateTimeUtc();
}

void QServer::heartbeat_error(QNetworkReply::NetworkError error) {
    auto reply = static_cast<QNetworkReply *>(sender());
    logger.error(
        Concern::Server,
        QString("Heartbeat could not be sent: (error %1: %2) %3")
                .arg(error)
                .arg(reply->errorString())
                .arg(QString(reply->readAll())
        )
    );
}

void QServer::heartbeat_ok(QNetworkReply * reply) {
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

void QServer::send_sightings(QVector<Sighting> sightings) const {
    for (auto & sighting: sightings) {
        this->send_sighting(sighting);
        sighting.debug();
    }
}

void QServer::send_sighting(const Sighting & sighting) const {
    logger.debug(Concern::Server, QString("Sending sighting %1 to %2").arg(sighting.prefix(), this->m_url_sighting.toString()));

    QHttpMultiPart * multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    multipart->append(sighting.jpg_part());
    multipart->append(sighting.xml_part());
    multipart->append(sighting.json());

    QNetworkRequest request(this->m_url_sighting);
    QNetworkReply * reply = this->m_network_manager->post(request, multipart);
    multipart->setParent(reply); // delete the multiPart with the reply
}

void QServer::button_send_heartbeat(void) {
    logger.info(Concern::Server, "Requesting a manual heartbeat");
    emit this->request_heartbeat();
}

void QServer::display_countdown(void) {
    this->ui->lb_countdown->setText(QString("%1 s")
        .arg(QStation::HeartbeatInterval / 1000 - this->m_last_heartbeat.secsTo(QDateTime::currentDateTimeUtc())));
}

bool QServer::is_changed(void) const {
    return (this->is_id_changed() || this->is_address_changed());
}

bool QServer::is_id_changed(void) const {
    return (this->ui->le_station_id->text() != this->station_id());
}

bool QServer::is_address_changed(void) const {
    return (
        (this->ui->le_ip->text() != this->address().toString()) ||
        (this->ui->sb_port->value() != this->port())
    );
}

void QServer::apply_changes_inner(void) {
    if (this->ui->le_station_id->text() != this->station_id()) {
        this->set_station_id(this->ui->le_station_id->text());
    }
    if ((this->ui->le_ip->text() != this->address().toString()) || (this->ui->sb_port->value() != this->port())) {
        this->set_address(this->ui->le_ip->text(), this->ui->sb_port->value());
    }
}

void QServer::discard_changes_inner(void) {
    this->ui->le_ip->setText(this->address().toString());
    this->ui->sb_port->setValue(this->port());
    this->ui->le_station_id->setText(this->station_id());
}
