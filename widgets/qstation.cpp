#include "include.h"

#include "qstation.h"
#include "ui_qstation.h"

extern EventLogger logger;
extern QSettings * settings;

const StationState QStation::Daylight           = StationState('D', "daylight", Icon::Daylight, "not observing, daylight");
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
    return QString("hsv(%1, %2, %3)").arg(h).arg(s).arg(v);
}

QStation::QStation(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QStation),
    m_state(QStation::NotObserving)
{
    ui->setupUi(this);

    this->m_state_logger = new StateLogger(this, "state.log");
    this->m_state_logger->initialize();

    this->m_timer_automatic = new QTimer(this);
    this->m_timer_automatic->setInterval(1000);
    this->connect(this->m_timer_automatic, &QTimer::timeout, this, &QStation::automatic_check);
    this->m_timer_automatic->start();

    this->m_timer_file_watchdog = new QTimer(this);
    this->m_timer_file_watchdog->setInterval(2000);
    this->connect(this->m_timer_file_watchdog, &QTimer::timeout, this, &QStation::file_check);
    this->m_timer_file_watchdog->start();

    this->m_timer_heartbeat = new QTimer(this);
    this->m_timer_heartbeat->setInterval(QStation::HeartbeatInterval);
    this->connect(this->m_timer_heartbeat, &QTimer::timeout, this, &QStation::heartbeat);
    this->m_timer_heartbeat->start();

    this->connect(this->ui->dsb_latitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QStation::handle_settings_changed);
    this->connect(this->ui->dsb_longitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QStation::handle_settings_changed);
    this->connect(this->ui->dsb_altitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QStation::handle_settings_changed);
    this->connect(this->ui->dsb_darkness_limit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QStation::handle_settings_changed);

    this->connect(this->ui->bt_apply, &QPushButton::clicked, this, &QStation::apply_settings);
    this->connect(this->ui->bt_discard, &QPushButton::clicked, this, &QStation::discard_settings);

    this->connect(this, &QStation::settings_changed, this, &QStation::discard_settings);
    this->connect(this, &QStation::settings_changed, this, &QStation::save_settings);
}

QStation::~QStation() {
    delete ui;
}

void QStation::initialize(void) {
    this->load_settings();
}

void QStation::load_settings(void) {
    try {
        this->load_settings_inner();
    } catch (ConfigurationError &e) {
        this->load_defaults();
    }
    this->discard_settings();

    this->send_heartbeat();
}

void QStation::load_settings_inner(void) {
    this->set_manual_control(
        settings->value("manual", false).toBool()
    );
    this->set_position(
        settings->value("station/latitude", 48.0).toDouble(),
        settings->value("station/longitude", 17.0).toDouble(),
        settings->value("station/altitude", 186.0).toDouble()
    );
    this->set_darkness_limit(
        settings->value("limits/darkness", -12.0).toDouble()
    );
}

void QStation::load_defaults(void) {
    this->set_manual_control(false);
    this->set_position(48.0, 17.0, 186.0);
    this->set_darkness_limit(-12.0);
}

void QStation::save_settings(void) const {
    settings->setValue("manual", this->is_manual());
    settings->setValue("station/latitude", this->latitude());
    settings->setValue("station/longitude", this->longitude());
    settings->setValue("station/altitude", this->altitude());
    settings->setValue("limits/darkness", this->darkness_limit());
    settings->sync();
}

// Manual control
void QStation::set_manual_control(bool manual) {
    logger.info(Concern::Operation, QString("Control set to %1").arg(manual ? "manual" : "automatic"));
    this->ui->cb_manual->setCheckState(manual ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    this->ui->cb_safety_override->setEnabled(manual);
    this->m_manual_control = manual;

    emit this->manual_mode_changed(manual);
}

bool QStation::is_manual(void) const { return this->m_manual_control; }

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
bool QStation::is_safety_overridden(void) const { return this->m_safety_override; }

/**
 * @brief QStation::set_position
 *  Set position of the station, with validation
 * @param new_latitude
 * @param new_longitude
 * @param new_altitude
 */
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

double QStation::latitude(void) const { return this->m_latitude; }
double QStation::longitude(void) const { return this->m_longitude; }
double QStation::altitude(void) const { return this->m_altitude; }

// Darkness limit settings
bool QStation::is_dark(const QDateTime & time) const {
    return (this->sun_altitude(time) < this->m_darkness_limit);
}

double QStation::darkness_limit(void) const { return this->m_darkness_limit; }

void QStation::set_darkness_limit(const double new_darkness_limit) {
    if ((new_darkness_limit < -18) || (new_darkness_limit > 0)) {
        throw ConfigurationError(QString("Darkness limit out of range: %1°").arg(new_darkness_limit));
    }

    this->m_darkness_limit = new_darkness_limit;
    logger.info(Concern::Configuration, QString("Station's darkness limit set to %1°").arg(this->m_darkness_limit));

    emit this->darkness_limit_changed(new_darkness_limit);
}

// Dome
void QStation::set_dome(QDome * const dome) { this->m_dome = dome; }
QDome* QStation::dome(void) const { return this->m_dome; }

// Scanner
void QStation::set_scanner(QScannerBox * const scanner) { this->m_scanner = scanner; }
QScannerBox* QStation::scanner(void) const { return this->m_scanner; }

// Storages
void QStation::set_storages(QStorageBox * const primary_storage, QStorageBox * const permanent_storage) {
    this->m_primary_storage = primary_storage;
    this->m_permanent_storage = permanent_storage;
}

QStorageBox * QStation::primary_storage(void) const { return this->m_primary_storage; }
QStorageBox * QStation::permanent_storage(void) const { return this->m_permanent_storage; }

// UFO manager
void QStation::set_ufo_manager(QUfoManager * const ufo_manager) { this->m_ufo_manager = ufo_manager; }
QUfoManager * QStation::ufo_manager(void) const { return this->m_ufo_manager; }

// Server getters and setters
void QStation::set_server(QServer * const server) { this->m_server = server; }
QServer * QStation::server(void) const { return this->m_server; }

/** Sun functions **/
Polar QStation::sun_position(const QDateTime & time) const {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + this->m_longitude * Rad;

    Vec3D equatorial = Universe::compute_sun_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], this->m_latitude * Rad, alt, az);

    return Polar(az + pi, alt);
}

Polar QStation::moon_position(const QDateTime & time) const {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + this->m_longitude * Rad;

    Vec3D equatorial = Universe::compute_moon_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], this->m_latitude * Rad, alt, az);

    return Polar(fmod(az + pi, 2 * pi), alt);
}

double QStation::sun_altitude(const QDateTime & time) const {
    return this->sun_position(time).theta * Deg;
}

double QStation::sun_azimuth(const QDateTime & time) const {
    return fmod(this->sun_position(time).phi * Deg + 360.0, 360.0);
}

double QStation::moon_altitude(const QDateTime & time) const {
    return this->moon_position(time).theta * Deg;
}

double QStation::moon_azimuth(const QDateTime & time) const {
    return fmod(this->moon_position(time).phi * Deg + 360.0, 360.0);
}

/* Compute next crossing of the Sun through the almucantar `altitude` in the specified direction (up/down)
 * with resolution of `resolution` seconds (optional, default = 60) */
QDateTime QStation::next_sun_crossing(double altitude, bool direction_up, int resolution) const {
    QDateTime now = QDateTime::fromTime_t((QDateTime::currentDateTimeUtc().toTime_t() / 60) * 60);
    double oldalt = this->sun_altitude(now);

    for (int i = 1; i < 86400 / resolution; ++i) {
        QDateTime moment = now.addSecs(resolution * i);
        double newalt = this->sun_altitude(moment);
        if (direction_up) {
            if ((oldalt < altitude) && (newalt > altitude)) {
                return moment;
            }
        } else {
            if ((oldalt > altitude) && (newalt < altitude)) {
                return moment;
            }
        }
        oldalt = newalt;
    }
    return QDateTime();
}

// Perform automatic state checks
void QStation::automatic_check(void) {
    this->ufo_manager()->auto_action(this->is_dark());

    const DomeStateS &stateS = this->dome()->state_S();

    if (!stateS.is_valid()) {
        logger.debug_error(Concern::Automatic, "S state is not valid, automatic loop skipped");
        this->set_state(QStation::DomeUnreachable);
        return;
    }

    // Emergency: close the cover, unless safety overridden
    if (stateS.dome_open_sensor_active() && !this->is_dark()) {
        if (this->is_safety_overridden()) {
            logger.debug(Concern::Automatic, "Emergency override active, not closing");
        } else {
            logger.warning(Concern::Automatic, "Closed the cover (not dark enough)");
            this->dome()->close_cover();
        }
    }

    // Emergency: turn off the image intensifier, unless safety overridden
    if (stateS.intensifier_active() && !this->is_dark()) {
        if (this->is_safety_overridden()) {
            logger.debug(Concern::Automatic, "Emergency override active, not turning off the intensifier");
        } else {
            logger.warning(Concern::Automatic, "Turned off the image intensifier (not dark enough)");
            this->dome()->turn_off_intensifier();
        }
    }

    if (this->m_manual_control) {
        // If we are in manual mode, pass other automatic checks
        logger.debug(Concern::Automatic, "Manual control mode, passing");
        this->set_state(QStation::Manual);
    } else {
        if (this->is_dark()) {
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
                        this->m_dome->open_cover();
                    }
                }

                // If the dome is closed, also turn off the intensifier
                if (stateS.intensifier_active()) {
                    logger.info(Concern::Automatic, "Cover is closed, turned off the intensifier");
                    this->m_dome->turn_off_intensifier();
                    this->set_state(QStation::NotObserving);
                }
            } else {
                if (stateS.dome_open_sensor_active()) {
                    // If the dome is open, turn on the image intensifier and the fan
                    if (stateS.intensifier_active()) {
                        logger.detail(Concern::Automatic, "Intensifier active");
                        this->set_state(QStation::Observing);
                    } else {
                        logger.info(Concern::Automatic, "Cover open, turned on the image intensifier");
                        this->m_dome->turn_on_intensifier();
                    }

                    if (!stateS.fan_active()) {
                        logger.info(Concern::Automatic, "Turned on the fan");
                        this->m_dome->turn_on_fan();
                    }

                    // But if humidity is very high, close the cover
                    if (this->dome()->is_very_humid()) {
                        logger.info(Concern::Automatic, "Closed the cover (high humidity)");
                        this->set_state(QStation::RainOrHumid);
                        this->m_dome->close_cover();
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

void QStation::process_sightings(QVector<Sighting> sightings) {
    logger.debug(Concern::Sightings, "Processing sightings...");
    for (auto &sighting: sightings) {
        this->m_server->send_sighting(sighting);
        this->m_permanent_storage->store_sighting(sighting, false);
        this->m_primary_storage->store_sighting(sighting, true);
    }
}

QJsonObject QStation::prepare_heartbeat(void) const {
     return QJsonObject {
        {"auto", !this->is_manual()},
        {"time", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"dome", this->dome()->json()},
        {"st", QString(this->state().code())},
        {"disk", QJsonObject {
            {"prim", this->primary_storage()->json()},
            {"perm", this->permanent_storage()->json()},
        }},
        {"cfg", QJsonObject {
            {"dl", this->darkness_limit()},
            {"hll", this->dome()->humidity_limit_lower()},
            {"hlu", this->dome()->humidity_limit_upper()},
        }},
    };
}

void QStation::heartbeat(void) {
    this->log_state();
    this->send_heartbeat();
}

void QStation::send_heartbeat(void) {
    this->m_server->send_heartbeat(this->prepare_heartbeat());
}

void QStation::set_state(StationState new_state) {
    if (new_state != this->m_state) {
        logger.debug(Concern::Operation, QString("State changed from %1 to %2").arg(this->state().display_string(), new_state.display_string()));
        this->m_state = new_state;
        emit this->state_changed(this->m_state);
    }
}

StationState QStation::state(void) const { return this->m_state; }

void QStation::file_check(void) {
    this->m_ufo_manager->auto_action(this->is_dark());
}

QString QStation::state_logger_filename(void) const { return this->m_state_logger->filename(); }

void QStation::log_state(void) {
    this->m_state_logger->log(QString("%1° %2 %3")
                              .arg(this->sun_altitude(), 5, 'f', 1)
                              .arg(QString(this->state().code()))
                              .arg(this->m_dome->status_line()));
}

// Event handlers
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
        (this->ui->dsb_latitude->value() != this->latitude()) ||
        (this->ui->dsb_longitude->value() != this->longitude()) ||
        (this->ui->dsb_altitude->value() != this->altitude()) ||
        (this->ui->dsb_darkness_limit->value() != this->darkness_limit())
    );
}

void QStation::apply_settings_inner(void) {
    if (
        (this->ui->dsb_latitude->value() != this->latitude()) ||
        (this->ui->dsb_longitude->value() != this->longitude()) ||
        (this->ui->dsb_altitude->value() != this->altitude())
    ) {
        this->set_position(this->ui->dsb_latitude->value(), this->ui->dsb_longitude->value(), this->ui->dsb_altitude->value());
    }

    if (this->ui->dsb_darkness_limit->value() != this->darkness_limit()) {
        this->set_darkness_limit(this->ui->dsb_darkness_limit->value());
    }
}

void QStation::discard_settings(void) {
    this->ui->dsb_latitude->setValue(this->latitude());
    this->ui->dsb_longitude->setValue(this->longitude());
    this->ui->dsb_altitude->setValue(this->altitude());
    this->ui->dsb_darkness_limit->setValue(this->darkness_limit());
}

void QStation::apply_settings(void) {
    try {
        this->apply_settings_inner();
        emit this->settings_changed();
    } catch (ConfigurationError &e) {
        logger.error(Concern::Configuration, e.what());
    }
    this->handle_settings_changed();
}

void QStation::handle_settings_changed(void) {
    bool changed = this->is_changed();

    this->ui->bt_apply->setText(QString("%1 changes").arg(changed ? "Apply" : "No"));
    this->ui->bt_apply->setEnabled(changed);
    this->ui->bt_discard->setEnabled(changed);
}
