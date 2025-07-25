#include <QTimer>
#include <QJsonObject>

#include "qstation.h"
#include "ui_qstation.h"
#include "utils/universe.h"
#include "utils/exceptions.h"

extern EventLogger logger;
extern QSettings * settings;


const StationState QStation::Daylight           = StationState('D', "daylight", Icon::Daylight, "not observing: too light");
const StationState QStation::Observing          = StationState('O', "observing", Icon::Observing, "observation in progress");
const StationState QStation::NotObserving       = StationState('N', "not observing", Icon::NotObserving, "not observing");
const StationState QStation::Manual             = StationState('M', "manual", Icon::Manual, "manual control enabled");
const StationState QStation::DomeUnreachable    = StationState('U', "dome unreachable", Icon::Failure, "dome is not responding");
const StationState QStation::RainOrHumid        = StationState('R', "rain or high humidity", Icon::NotObserving, "rain sensor active or humidity too high");
const StationState QStation::NoMasterPower      = StationState('P', "no master power", Icon::NotObserving, "master power sensor inactive");
const StationState QStation::Inconsistent       = StationState('I', "inconsistent", Icon::Failure, "inconsistent state");

QString QStation::temperature_colour(float temperature) {
    float h = 0, s = 0, v = 0;
    if (temperature < 0.0) {
        h = 180 - 4.0 * temperature;
        s = (1 + temperature / 50.0) * 255;
        v = 200;
    } else {
        if (temperature < 15) {
            h = 180 - 6.0 * temperature;
        } else {
            h = 90 - (4.5 * (temperature - 15));
        }
        if (h < 0) {
            h += 360;
        }
        s = 255;
        v = 160;
    }
    return QString("hsv(%1, %2, %3)").arg(h, s, v);
}

QStation::QStation(QWidget * parent):
    QAmosWidget(parent),
    ui(new Ui::QStation),
    m_start_time(QDateTime::currentDateTimeUtc()),
    m_latitude(49.0),
    m_longitude(18.0),
    m_altitude(0.0),
    m_manual_control(false),
    m_safety_override(false),
    m_state(QStation::DomeUnreachable)
{
    ui->setupUi(this);

    this->m_state_logger = new StateLogger(this, "state.log");
    this->m_state_logger->initialize();

    this->m_timer_automatic = new QTimer(this);
    this->m_timer_automatic->setInterval(1000);
    this->connect(this->m_timer_automatic, &QTimer::timeout, this, &QStation::automatic_timer);
    this->m_timer_automatic->start();
}

QStation::~QStation() {
    delete this->m_state_logger;
    delete ui;
}

void QStation::initialize(QSettings * settings) {
    QAmosWidget::initialize(settings);
}

void QStation::connect_slots(void) {
    for (auto && widget: {this->ui->dsb_latitude, this->ui->dsb_longitude, this->ui->dsb_altitude}) {
        this->connect(widget, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QStation::settings_changed);
    }
}

void QStation::load_settings_inner(void) {
    this->set_manual_control(
        this->m_settings->value("manual", false).toBool()
    );
    this->set_position(
        this->m_settings->value("station/latitude", QStation::DefaultLatitude).toDouble(),
        this->m_settings->value("station/longitude", QStation::DefaultLongitude).toDouble(),
        this->m_settings->value("station/altitude", QStation::DefaultAltitude).toDouble()
    );
}

void QStation::load_defaults(void) {
    this->set_manual_control(false);
    this->set_position(QStation::DefaultLatitude, QStation::DefaultLongitude, QStation::DefaultAltitude);
}

void QStation::save_settings_inner(void) const {
    this->m_settings->setValue("manual", this->is_manual());
    this->m_settings->setValue("station/latitude", this->latitude());
    this->m_settings->setValue("station/longitude", this->longitude());
    this->m_settings->setValue("station/altitude", this->altitude());
}

// Manual control
void QStation::set_manual_control(bool manual) {
    logger.info(Concern::Operation, QString("Control set to %1").arg(manual ? "manual" : "automatic"));

    this->ui->cb_manual->setCheckState(manual ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    this->ui->cb_safety_override->setEnabled(manual);
    this->ui->cb_safety_override->setCheckState(manual ? this->ui->cb_safety_override->checkState() : Qt::CheckState::Unchecked);
    this->m_manual_control = manual;

    emit this->manual_mode_changed(manual);
}

// Safety override
void QStation::set_safety_override(bool override) {
    if (this->m_manual_control) {
        logger.warning(Concern::Operation, QString("Safety override %1abled").arg(override ? "en" : "dis"));
        this->m_safety_override = override;
        this->ui->cb_safety_override->setCheckState(override ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

        emit this->safety_override_changed(override);
    } else {
        logger.error(Concern::Operation, "Cannot override safety when not in manual mode!");
    }
}

void QStation::set_position(const double new_latitude, const double new_longitude, const double new_altitude) {
    // Check that the new values are meaningful
    if (fabs(new_latitude) > 90) {
        throw ConfigurationError(QString("Latitude out of range: %1").arg(new_latitude));
    }
    if (fabs(new_longitude) > 180) {
        throw ConfigurationError(QString("Longitude out of range: %1").arg(new_longitude));
    }
    if ((new_altitude < -400) || (new_altitude > 13000)) {
        throw ConfigurationError(QString("Altitude out of range: %1").arg(new_altitude));
    }

    this->m_latitude = new_latitude;
    this->m_longitude = new_longitude;
    this->m_altitude = new_altitude;

    logger.info(Concern::Configuration, QString("Station position set to %1°, %2°, %3 m")
                .arg(this->m_latitude, 0, 'f', 6)
                .arg(this->m_longitude, 0, 'f', 6)
                .arg(this->m_altitude, 0, 'f', 1));

    emit this->position_changed();
}

/** Darkness limit settings **/
bool QStation::is_dark_allsky(const QDateTime & time) const {
    return (this->sun_altitude(time) < this->camera_allsky()->darkness_limit());
}

bool QStation::is_dark_spectral(const QDateTime & time) const {
    return (this->sun_altitude(time) < this->camera_spectral()->darkness_limit());
}

/** Pointers to other relevant widgets **/
void QStation::set_cameras(const QCamera * const allsky, const QCamera * const spectral) {
    this->m_camera_allsky = allsky;
    this->m_camera_spectral = spectral;
}

void QStation::set_server(const QServer * const server) {
    this->m_server = server;
    this->connect(this->m_server, &QServer::request_heartbeat, this, &QStation::send_heartbeat);
}

/** Sun functions **/
Polar QStation::sun_position(const QDateTime & time) const {
    return Universe::sun_position(this->latitude(), this->longitude(), time);
}

Polar QStation::moon_position(const QDateTime & time) const {
    return Universe::moon_position(this->latitude(), this->longitude(), time);
}

double QStation::sun_altitude(const QDateTime & time) const {
    return Universe::sun_altitude(this->latitude(), this->longitude(), time);
}

double QStation::sun_azimuth(const QDateTime & time) const {
    return Universe::sun_azimuth(this->latitude(), this->longitude(), time);
}

double QStation::moon_altitude(const QDateTime & time) const {
    return Universe::moon_altitude(this->latitude(), this->longitude(), time);
}

double QStation::moon_azimuth(const QDateTime & time) const {
    return Universe::moon_azimuth(this->latitude(), this->longitude(), time);
}

/* Compute next crossing of the Sun through the almucantar `altitude` in the specified direction (up/down)
 * with resolution of `resolution` seconds (optional, default = 60) */
QDateTime QStation::next_sun_crossing(double altitude, bool direction_up, int resolution) const {
    return Universe::next_crossing(Universe::sun_altitude, this->latitude(), this->longitude(), altitude, direction_up, resolution);
}

/* The same with the Moon */
QDateTime QStation::next_moon_crossing(double altitude, bool direction_up, int resolution) const {
    return Universe::next_crossing(Universe::moon_altitude, this->latitude(), this->longitude(), altitude, direction_up, resolution);
}

void QStation::automatic_timer(void) {
    emit this->automatic_action_allsky(this->is_dark_allsky(), this->dome()->open_since());
    emit this->automatic_action_spectral(this->is_dark_spectral(), QDateTime(QDate(2020, 1, 1), QTime(0, 0, 0, 0)));
}

// Perform automatic state checks
void QStation::automatic_cover(void) {
    logger.debug(Concern::Automatic, "Automatic cover action");
    const DomeStateS & stateS = this->dome()->state_S();

    if (!stateS.is_valid()) {
        logger.debug_error(Concern::Automatic, "S state is not valid, automatic loop skipped");
        this->set_state(QStation::DomeUnreachable);
        return;
    }

    // Emergency: close the cover, unless safety overridden
    if (stateS.dome_open_sensor_active() && !this->is_dark_allsky()) {
        if (this->is_safety_overridden()) {
            logger.debug(Concern::Automatic, "Emergency override active, not closing");
        } else {
            logger.info(Concern::Automatic, "Closed the cover (not dark enough)");
            this->dome()->close_cover();
        }
    }

    // Emergency: turn off the image intensifier, unless safety overridden
    if (stateS.intensifier_active() && !this->is_dark_allsky()) {
        if (this->is_safety_overridden()) {
            logger.debug(Concern::Automatic, "Emergency override active, not turning off the intensifier");
        } else {
            logger.info(Concern::Automatic, "Turned off the image intensifier (not dark enough)");
            this->dome()->turn_off_intensifier();
        }
    }

    // Inconsistent state: both dome sensors active
    if (stateS.dome_closed_sensor_active() && stateS.dome_open_sensor_active()) {
        logger.debug_error(Concern::Operation, "Both open-dome and closed-dome sensors active, emergency closing");
        this->set_state(QStation::Inconsistent);
        this->dome()->close_cover();
        this->dome()->turn_off_intensifier();
    } else {
        if (this->m_manual_control) {
            // If we are in manual mode, pass other automatic checks
            logger.debug(Concern::Automatic, "Manual control mode, passing");
            this->set_state(QStation::Manual);
        } else {
            if (this->is_dark_allsky()) {
                if (stateS.dome_closed_sensor_active()) {
                    if (stateS.rain_sensor_active()) {
                        logger.debug(Concern::Automatic, "Will not open: raining");
                        this->set_state(QStation::RainOrHumid);
                    } else {
                        if (this->dome()->is_humid()) {
                            logger.debug(Concern::Automatic, "Will not open: humidity is too high");
                            this->set_state(QStation::RainOrHumid);
                        } else {
                            logger.info(Concern::Automatic, "Opening the cover");
                            this->dome()->open_cover();
                        }
                    }

                    // If the dome is closed, also turn off the intensifier
                    if (stateS.intensifier_active()) {
                        logger.info(Concern::Automatic, "Cover is closed, turned off the intensifier");
                        this->dome()->turn_off_intensifier();
                        this->set_state(QStation::NotObserving);
                    }
                } else {
                    if (stateS.dome_open_sensor_active()) {
                        // If the dome is open, turn on the image intensifier and the fan
                        if (stateS.intensifier_active()) {
                            logger.detail(Concern::Automatic, "Intensifier is active");
                            this->set_state(QStation::Observing);
                        } else {
                            logger.info(Concern::Automatic, "Cover open, turned on the image intensifier");
                            this->dome()->turn_on_intensifier();
                        }

                        if (!stateS.fan_active()) {
                            logger.info(Concern::Automatic, "Turned on the fan");
                            this->dome()->turn_on_fan();
                        }

                        // But if humidity is very high, close the cover
                        if (this->dome()->is_very_humid()) {
                            logger.info(Concern::Automatic, "Closed the cover (high humidity)");
                            this->set_state(QStation::RainOrHumid);
                            this->dome()->close_cover();
                        }
                    } else {
                        this->set_state(QStation::NotObserving);
                    }
                }
            } else {
                logger.debug(Concern::Automatic, "Will not open: daylight");
                this->set_state(QStation::Daylight);
            }
        }
    }
}

QJsonObject QStation::json(void) const {
    return QJsonObject {
        {"protocol", HEARTBEAT_PROTOCOL_VERSION},
        {"auto", !this->is_manual()},
        {"start", this->m_start_time.toString(Qt::ISODate)},
        {"time", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"st", QString(QChar(this->state().code()))},
        {"dome", this->dome()->json()},
        {"cas", this->camera_allsky()->json()},
        {"csp", this->camera_spectral()->json()},
        {"cfg", QJsonObject {
            {"hll", this->dome()->humidity_limit_lower()},
            {"hlu", this->dome()->humidity_limit_upper()},
        }},
        {"pos", QJsonObject {
            {"lat", this->latitude()},
            {"lon", this->longitude()},
            {"alt", this->altitude()},
        }},
#if PROTOCOL == 2015
        {"cv", QString("%1%2").arg(VERSION_STRING, "sc")},
#elif PROTOCOL == 2020
        {"cv", VERSION_STRING},
#endif
        {"cs", this->start_time().toString(Qt::ISODate)},
    };
}

/** Heartbeat action: log state and send **/
void QStation::heartbeat(void) const {
    this->log_state();
    this->send_heartbeat();
}

/** Actually send a heartbeat through the server **/
void QStation::send_heartbeat(void) const {
    logger.debug(Concern::Heartbeat, "Sending a heartbeat...");
    this->m_server->send_heartbeat(this->json());
}

void QStation::set_state(StationState new_state) {
    if (new_state != this->m_state) {
        logger.info(Concern::Operation, QString("State changed from \"%1\" to \"%2\"")
                     .arg(this->state().display_string(), new_state.display_string()));
        this->m_state = new_state;
        emit this->state_changed(this->m_state);
    }
}

void QStation::log_state(void) const {
    this->m_state_logger->log(QString("%1° %2 %3")
                              .arg(this->sun_altitude(), 5, 'f', 1)
                              .arg(QString(QChar(this->state().code())), this->dome()->status_line()));
}

/*********************** Event handlers ***********************************/
void QStation::on_cb_manual_clicked(bool checked) {
    this->set_manual_control(checked);
    this->save_settings();
}

void QStation::on_cb_safety_override_clicked(bool checked) {
    if (checked) {
        QMessageBox box(
                        QMessageBox::Icon::Warning,
                        "Safety mechanism override",
                        "You are about to override the safety mechanisms!",
                        QMessageBox::Ok | QMessageBox::Cancel,
                        this
                    );
        box.setInformativeText("Turning on the image intensifier during the day with open cover may result in permanent damage.");
        box.setWindowIcon(QIcon(":/images/blue.ico"));
        QMessageBox::StandardButton choice = (QMessageBox::StandardButton) box.exec();

        switch (choice) {
            case QMessageBox::Ok:
                this->set_safety_override(true);
                break;
            case QMessageBox::Cancel:
            default:
                this->ui->cb_safety_override->setCheckState(Qt::CheckState::Unchecked);
                break;
        }
    } else {
        this->set_safety_override(false);
    }
}

/**************************** Change handlers *****************************/
bool QStation::is_changed(void) const {
    return (
        (abs(this->ui->dsb_latitude->value() - this->latitude()) > 1e-9) ||
        (abs(this->ui->dsb_longitude->value() - this->longitude()) > 1e-9) ||
        (abs(this->ui->dsb_altitude->value() - this->altitude()) > 1e-3)
    );
}

void QStation::apply_changes_inner(void) {
    if (this->is_changed()) {
        this->set_position(this->ui->dsb_latitude->value(), this->ui->dsb_longitude->value(), this->ui->dsb_altitude->value());
    }
    emit this->ui->dsb_latitude->valueChanged(this->latitude());
    emit this->ui->dsb_longitude->valueChanged(this->longitude());
    emit this->ui->dsb_altitude->valueChanged(this->altitude());
}

void QStation::discard_changes_inner(void) {
    this->ui->dsb_latitude->setValue(this->latitude());
    this->ui->dsb_longitude->setValue(this->longitude());
    this->ui->dsb_altitude->setValue(this->altitude());
}

void QStation::on_dsb_latitude_valueChanged(double value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->dsb_latitude, this->latitude(), value);
}


void QStation::on_dsb_longitude_valueChanged(double value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->dsb_longitude, this->longitude(), value);
}


void QStation::on_dsb_altitude_valueChanged(double value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->dsb_altitude, this->altitude(), value);
}

