#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

#include "logging/eventlogger.h"
#include "widgets/qstation.h"
#include "widgets/qserver.h"
#include "utils/exceptions.h"

#include "ui_qserver.h"

extern EventLogger logger;
extern QSettings * settings;


QServer::QServer(QWidget * parent):
    QAmosWidget(parent),
    ui(new Ui::QServer)
{
    this->ui->setupUi(this);

    this->m_heartbeat_manager = new QNetworkAccessManager(this);
    this->m_sighting_manager = new QNetworkAccessManager(this);
    this->connect(this->ui->bt_send_heartbeat, &QPushButton::clicked, this, &QServer::button_send_heartbeat);
    this->connect(this, &QServer::settings_saved, this, &QServer::refresh_urls);
    this->connect(this->m_heartbeat_manager, &QNetworkAccessManager::finished, this, &QServer::heartbeat_finished);
    this->connect(this->m_sighting_manager, &QNetworkAccessManager::finished, this, &QServer::sighting_received);

    this->m_timer_heartbeat = new QTimer(this);
    this->m_timer_heartbeat->setInterval(60000);
    this->m_timer_heartbeat->start();
}

QServer::~QServer() {
    delete this->ui;
    delete this->m_heartbeat_manager;
    delete this->m_sighting_manager;
}

void QServer::initialize(QSettings * settings) {
    QAmosWidget::initialize(settings);
    this->refresh_urls();
}

void QServer::connect_slots(void) {
    this->connect(this->ui->le_station_id, &QLineEdit::textChanged, this, &QServer::settings_changed);
    this->connect(this->ui->le_ip, &QLineEdit::textChanged, this, &QServer::settings_changed);
    this->connect(this->ui->sb_port, QOverload<int>::of(&QSpinBox::valueChanged), this, &QServer::settings_changed);
    this->connect(this->ui->sb_interval, QOverload<int>::of(&QSpinBox::valueChanged), this, &QServer::settings_changed);
}

void QServer::load_settings_inner(void) {
    this->set_station_id(
        this->m_settings->value("station/id", "none").toString()
    );
    this->set_address(
        this->m_settings->value("server/ip", "127.0.0.1").toString(),
        this->m_settings->value("server/port", 4805).toInt()
    );
    this->set_heartbeat_interval(
        this->m_settings->value("server/interval", 60).toInt()
    );
    this->refresh_urls();
}

void QServer::load_defaults(void) {
    this->set_station_id("none");
    this->set_address("127.0.0.1", 4805);
    this->set_heartbeat_interval(60);
}

void QServer::save_settings_inner(void) const {
    this->m_settings->setValue("station/id", this->station_id());
    this->m_settings->setValue("server/ip", this->address().toString());
    this->m_settings->setValue("server/port", this->port());
    this->m_settings->setValue("server/interval", this->heartbeat_interval());
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

void QServer::set_heartbeat_interval(unsigned int interval) {
    this->m_heartbeat_interval = interval;
    this->m_last_heartbeat = QDateTime::currentDateTime();
    logger.info(Concern::Server, QString("Heartbeat interval set to %1 s").arg(this->heartbeat_interval()));
    this->m_timer_heartbeat->stop();
    this->m_timer_heartbeat->setInterval(interval * 1000);
    this->m_timer_heartbeat->start();
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
    logger.debug(Concern::Heartbeat, QString("Sending a heartbeat to %1").arg(this->m_url_heartbeat.toString()));

    QNetworkRequest request(this->m_url_heartbeat);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray message = QJsonDocument(heartbeat).toJson(QJsonDocument::Compact);
    logger.debug(Concern::Heartbeat, QString("Heartbeat assembled: '%1'").arg(QString(message)));

    QNetworkReply * reply = this->m_heartbeat_manager->post(request, message);
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

void QServer::heartbeat_finished(QNetworkReply * reply) {
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        logger.debug(
            Concern::Server,
            QString("Heartbeat accepted (HTTP code %1), response \"%2\"").arg(
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString(),
                QString(reply->readAll())
            )
        );
        emit this->heartbeat_created();
    }
}

void QServer::send_sighting(const Sighting & sighting) const {
    logger.debug(Concern::Server, QString("Sending sighting '%1' to %2").arg(sighting.prefix(), this->m_url_sighting.toString()));

    QHttpMultiPart * multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    for (const auto & part: sighting.assemble()) {
        multipart->append(part);
    }

    QNetworkRequest request(this->m_url_sighting);
    QNetworkReply * reply = this->m_sighting_manager->post(request, multipart);
    reply->setProperty("sighting", sighting.prefix());
    multipart->setParent(reply); // delete the multipart with the reply

    emit this->sighting_sent(sighting.prefix());
}

void QServer::sighting_received(QNetworkReply * reply) {
    // Everything that is needed from the reply, read before it is scheduled for deletion
    const QString sighting_id = reply->property("sighting").toString();
    const QNetworkReply::NetworkError error = reply->error();
    const QVariant status_attribute = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const QString error_string = reply->errorString();
    const QString response = QString(reply->readAll());

    /* The multipart is parented to the reply and its parts hold every uploaded file in memory,
     * so a reply that is never deleted leaks the whole payload -- once per attempt.
     */
    reply->deleteLater();

    /* No HTTP response at all: the connection was refused, the host could not be resolved, the link
     * is down, or the transfer was cut mid-flight. Nothing has been said about the sighting itself,
     * so keep it and try again later.
     */
    if (!status_attribute.isValid()) {
        logger.debug_error(
            Concern::Server,
            QString("No response for sighting '%1', will try again (error %2: %3)")
                .arg(sighting_id)
                .arg(error)
                .arg(error_string)
        );
        emit this->sighting_error(sighting_id, error);
        return;
    }

    /* Otherwise go by the status code, which is the actual contract with the server. Dispatching on
     * QNetworkReply::NetworkError instead means reading that contract through Qt's statusCodeFromHttp
     * table, which is how 400 ended up in the default branch, 422 arrived disguised as an "unknown
     * content error", and branches were kept for 409 and 405 that the server never sends.
     */
    const int status = status_attribute.toInt();

    switch (status) {
        case 200:           // An existing sighting was updated, which is how a duplicate is reported
            [[fallthrough]];
        case 201: {         // A new sighting was created
            logger.info(
                Concern::Server,
                QString("Sighting '%1' created on the server (HTTP code %2)").arg(sighting_id).arg(status)
            );
            logger.debug(Concern::Server, QString("Response \"%1\"").arg(response));

            /* The body is a courtesy, not a contract: anything unparseable is still an acceptance.
             * "filename" is the metadata file the server stored, and null when no xml or yaml part
             * reached it -- which is the expected answer for an image delivered before its reduction
             * has run. A missing key means an older server that does not report it at all, and must
             * keep counting as stored. The warnings are the server's own remarks about the sighting,
             * and this is the only place an operator would ever see them.
             */
            bool metadata_stored = true;
            const QJsonObject body = QJsonDocument::fromJson(response.toUtf8()).object();

            if (body.contains("filename")) {
                metadata_stored = !body.value("filename").isNull();
            }

            for (const QJsonValue & warning: body.value("warnings").toArray()) {
                logger.warning(Concern::Server,
                               QString("Sighting '%1': %2").arg(sighting_id, warning.toString()));
            }

            emit this->sighting_accepted(sighting_id, metadata_stored);
            break;
        }
        case 400: {
            /* The server could not read the report. This client builds the metadata part
             * mechanically, so it is far more likely that the request never reached the view intact
             * -- an unlisted host, or a proxy rejecting a truncated body -- than that this sighting
             * is bad. Keep it and try again; quarantining here would throw away a whole night's
             * data over a server-side misconfiguration. Note the body may be HTML, not JSON.
             */
            logger.error(
                Concern::Server,
                QString("Sighting '%1' was rejected as malformed (HTTP 400), will try again: %2")
                    .arg(sighting_id, response)
            );
            emit this->sighting_error(sighting_id, error);
            break;
        }
        case 409: {
            // The sighting already exists on the server and this copy is redundant.
            logger.error(
                Concern::Server,
                QString("Sighting '%1' rejected as a duplicate (HTTP 409): %2").arg(sighting_id, response)
            );
            emit this->sighting_conflict(sighting_id, error);
            break;
        }
        case 422: {
            // The server read the report and will not take it. Retain it, but do not try again.
            logger.error(
                Concern::Server,
                QString("Sighting '%1' could not be processed (HTTP 422), will not be sent again: %2")
                    .arg(sighting_id, response)
            );
            emit this->sighting_conflict(sighting_id, error);
            break;
        }
        default: {
            /* Anything else -- 500 from the server, 413 from a proxy, a status the contract grows
             * later. None of them say this sighting is unacceptable, so keep it and try again.
             */
            logger.error(
                Concern::Server,
                QString("Sighting '%1' failed with HTTP %2, will try again: %3")
                    .arg(sighting_id).arg(status).arg(response)
            );
            emit this->sighting_error(sighting_id, error);
            break;
        }
    }
}

void QServer::button_send_heartbeat(void) {
    logger.info(Concern::Server, "Sending a heartbeat after an explicit request");
    emit this->request_heartbeat();
}

void QServer::display_countdown(void) {
    this->ui->lb_countdown->setText(QString("%1 s")
        .arg(this->heartbeat_interval() - this->m_last_heartbeat.secsTo(QDateTime::currentDateTimeUtc())));
}

bool QServer::is_changed(void) const {
    return (this->is_id_changed() || this->is_address_changed() || this->is_interval_changed());
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

bool QServer::is_interval_changed(void) const {
    return (this->ui->sb_interval->value() != this->heartbeat_interval());
}

void QServer::apply_changes_inner(void) {
    if (this->is_id_changed()) {
        this->set_station_id(this->ui->le_station_id->text());
    }
    if (this->is_address_changed()) {
        this->set_address(this->ui->le_ip->text(), this->ui->sb_port->value());
    }
    if (this->is_interval_changed()) {
        this->set_heartbeat_interval(this->ui->sb_interval->value());
    }
    emit this->ui->le_station_id->textChanged(this->station_id());
    emit this->ui->le_ip->textChanged(this->address().toString());
    emit this->ui->sb_port->valueChanged(this->port());
    emit this->ui->sb_interval->valueChanged(this->heartbeat_interval());
}

void QServer::discard_changes_inner(void) {
    this->ui->le_ip->setText(this->address().toString());
    this->ui->sb_port->setValue(this->port());
    this->ui->le_station_id->setText(this->station_id());
    this->ui->sb_interval->setValue(this->heartbeat_interval());
}

void QServer::on_le_station_id_textChanged(const QString & text) {
    Q_UNUSED(text);
    this->display_changed(this->ui->le_station_id, this->ui->le_station_id->text(), this->station_id());
}

void QServer::on_sb_interval_valueChanged(int value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->sb_interval, this->ui->sb_interval->value(), this->heartbeat_interval());
}

void QServer::on_le_ip_textChanged(const QString & text) {
    Q_UNUSED(text);
    this->display_changed(this->ui->le_ip, this->ui->le_ip->text(), this->address().toString());
}

void QServer::on_sb_port_valueChanged(int value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->sb_port, this->ui->sb_port->value(), this->port());
}

