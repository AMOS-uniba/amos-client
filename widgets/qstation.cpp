#include "include.h"

#include "qstation.h"
#include "ui_qstation.h"

extern EventLogger logger;
extern QSettings * settings;


const StationState QStation::Daylight           = StationState('D', "daylight", Icon::Daylight, "not observing during the day");
const StationState QStation::Observing          = StationState('O', "observing", Icon::Observing, "observation in progress");
const StationState QStation::NotObserving       = StationState('N', "not observing", Icon::NotObserving, "observation stopped");
const StationState QStation::Manual             = StationState('M', "manual", Icon::Manual, "manual control enabled");
const StationState QStation::DomeUnreachable    = StationState('U', "dome unreachable", Icon::Failure, "serial port has no connection");
const StationState QStation::RainOrHumid        = StationState('R', "rain or high humidity", Icon::NotObserving, "rain sensor active or humidity too high");
const StationState QStation::NoMasterPower      = StationState('P', "no master power", Icon::NotObserving, "master power sensor inactive");

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
    m_latitude(49.0),
    m_longitude(18.0),
    m_altitude(1.0),
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

    this->m_timer_heartbeat = new QTimer(this);
    this->m_timer_heartbeat->setInterval(QStation::HeartbeatInterval);
    this->connect(this->m_timer_heartbeat, &QTimer::timeout, this, &QStation::heartbeat);
    this->m_timer_heartbeat->start();
}

QStation::~QStation() {
    delete this->m_state_logger;
    delete ui;
}

void QStation::initialize(void) {
    QAmosWidget::initialize();
}

void QStation::connect_slots(void) {
    for (auto && widget: {this->ui->dsb_latitude, this->ui->dsb_longitude, this->ui->dsb_altitude}) {
        this->connect(widget, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QStation::settings_changed);
    }
}

void QStation::load_settings_inner(const QSettings * const settings) {
    this->set_manual_control(
        settings->value("manual", false).toBool()
    );
    this->set_position(
        settings->value("station/latitude", 48.0).toDouble(),
        settings->value("station/longitude", 17.0).toDouble(),
        settings->value("station/altitude", 186.0).toDouble()
    );
}

void QStation::load_defaults(void) {
    this->set_manual_control(false);
    this->set_position(48.0, 17.0, 186.0);
}

void QStation::save_settings_inner(QSettings * settings) const {
    settings->setValue("manual", this->is_manual());
    settings->setValue("station/latitude", this->latitude());
    settings->setValue("station/longitude", this->longitude());
    settings->setValue("station/altitude", this->altitude());
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
    return true;
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

void QStation::set_dome(const QDome * const dome) {
    this->m_dome = dome;
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
    return Universe::next_sun_crossing(this->latitude(), this->longitude(), altitude, direction_up, resolution);
}

void QStation::automatic_timer(void) {
    emit this->automatic_action_allsky(this->is_dark_allsky());
    emit this->automatic_action_spectral(this->is_dark_spectral());
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

QJsonObject QStation::json(void) const {
    return QJsonObject {
        {"auto", !this->is_manual()},
        {"time", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"st", QString(this->state().code())},
        {"dome", this->dome()->json()},
        {"cas", this->camera_allsky()->json()},
        {"csp", this->camera_spectral()->json()},
        {"cfg", QJsonObject {
            {"hll", this->dome()->humidity_limit_lower()},
            {"hlu", this->dome()->humidity_limit_upper()},
        }},
    };
}

void QStation::heartbeat(void) const {
    this->log_state();
    this->send_heartbeat();
}

void QStation::send_heartbeat(void) const {
    logger.debug(Concern::Heartbeat, "Sending a heartbeat...");
    this->m_server->send_heartbeat(this->json());
}

void QStation::set_state(StationState new_state) {
    if (new_state != this->m_state) {
        logger.debug(Concern::Operation, QString("State changed from \"%1\" to \"%2\"")
                     .arg(this->state().display_string(), new_state.display_string()));
        this->m_state = new_state;
        emit this->state_changed(this->m_state);
    }
}

StationState QStation::state(void) const { return this->m_state; }

QString QStation::state_logger_filename(void) const { return this->m_state_logger->filename(); }

void QStation::log_state(void) const {
    this->m_state_logger->log(QString("%1° %2 %3")
                              .arg(this->sun_altitude(), 5, 'f', 1)
                              .arg(QString(this->state().code()), this->dome()->status_line()));
}

/*********************** Event handlers ***********************************/
void QStation::on_cb_manual_clicked(bool checked) {
    this->set_manual_control(checked);
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
        int choice = box.exec();

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
}

void QStation::discard_changes_inner(void) {
    this->ui->dsb_latitude->setValue(this->latitude());
    this->ui->dsb_longitude->setValue(this->longitude());
    this->ui->dsb_altitude->setValue(this->altitude());
}
