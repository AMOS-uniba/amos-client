#include "include.h"
#include <windows.h>
#include <winuser.h>

extern EventLogger logger;

Station::Station(const QString& id): m_id(id), m_state(StationState::NOT_OBSERVING), m_ufo_path("") {
    qDebug() << "Station created";

    this->m_dome = new Dome();
    this->m_state_logger = new StateLogger(this, "state.log");

    this->m_server = nullptr;

    this->m_timer_automatic = new QTimer(this);
    this->m_timer_automatic->setInterval(1000);
    this->connect(this->m_timer_automatic, &QTimer::timeout, this, &Station::automatic_check);
    this->m_timer_automatic->start();

    this->m_timer_file_watchdog = new QTimer(this);
    this->m_timer_file_watchdog->setInterval(2000);
    this->connect(this->m_timer_file_watchdog, &QTimer::timeout, this, &Station::file_check);
    this->m_timer_file_watchdog->start();
}

Station::~Station(void) {
    delete this->m_dome;
    delete this->m_timer_automatic;
    delete this->m_primary_storage;
    delete this->m_permanent_storage;

    if (this->m_server != nullptr) {
        delete this->m_server;
    }
}

void Station::set_storages(const QDir& primary_storage_dir, const QDir& permanent_storage_dir) {
    this->m_primary_storage = new Storage("primary", primary_storage_dir);
    this->m_permanent_storage = new Storage("permanent", permanent_storage_dir);
}

Storage& Station::primary_storage(void) { return *this->m_primary_storage; }
Storage& Station::permanent_storage(void) { return *this->m_permanent_storage; }

Dome* Station::dome(void) const { return this->m_dome; }

// Server getters and setters
void Station::set_server(Server *server) {
    this->m_server = server;
}

Server* Station::server(void) { return this->m_server; }

// Position getters and setters

// Set position of the station
void Station::set_position(const double new_latitude, const double new_longitude, const double new_altitude) {
    // Check that the new values are meaningful
    if (fabs(new_latitude) > 90) {
        throw ConfigurationError(QString("Latitude out of range: %1").arg(new_latitude));
    }
    if (fabs(new_longitude) > 180) {
        throw ConfigurationError(QString("Longitude out of range: %1").arg(new_longitude));
    }
    if ((new_altitude < -400) || (new_altitude > 10000)) {
        throw ConfigurationError(QString("Altitude out of range: %1").arg(new_altitude));
    }

    this->m_latitude = new_latitude;
    this->m_longitude = new_longitude;
    this->m_altitude = new_altitude;
    logger.info(QString("Station position set to %1°, %2°, %3 m").arg(this->m_latitude).arg(this->m_longitude).arg(this->m_altitude));
}

double Station::latitude(void) const { return this->m_latitude; }
double Station::longitude(void) const { return this->m_longitude; }
double Station::altitude(void) const { return this->m_altitude; }

void Station::set_id(const QString& new_id) {
    if (new_id.length() > 4) {
        throw ConfigurationError(QString("Cannot set station id to '%1'").arg(new_id));
    }

    this->m_id = new_id;
    logger.info(QString("Station id set to '%1'").arg(this->m_id));
}

const QString& Station::get_id(void) const { return this->m_id; }

// Darkness limit settings
bool Station::is_dark(const QDateTime& time) const {
    return (this->sun_altitude(time) < this->m_darkness_limit);
}

double Station::darkness_limit(void) const { return this->m_darkness_limit; }

void Station::set_darkness_limit(const double new_darkness_limit) {
    if ((new_darkness_limit < -18) || (new_darkness_limit > 0)) {
        throw ConfigurationError(QString("Darkness limit out of range: %1°").arg(m_darkness_limit));
    }

    this->m_darkness_limit = new_darkness_limit;
    logger.info(QString("Station's darkness limit set to %1°").arg(m_darkness_limit));
}

// Humidity limit settings
bool Station::is_humid(void) const {
    return (this->m_dome->state_T().humidity_sht() >= this->m_humidity_limit_lower);
}

bool Station::is_very_humid(void) const {
    return (this->m_dome->state_T().humidity_sht() >= this->m_humidity_limit_upper);
}

double Station::humidity_limit_lower(void) const { return this->m_humidity_limit_lower; }
double Station::humidity_limit_upper(void) const { return this->m_humidity_limit_upper; }

void Station::set_humidity_limits(const double new_lower, const double new_upper) {
    if ((new_lower < 0) || (new_lower > 100) || (new_upper < 0) || (new_upper > 100) || (new_lower > new_upper - 1)) {
        throw ConfigurationError(QString("Invalid humidity limits: %1% - %2%").arg(new_lower).arg(new_upper));
    }

    this->m_humidity_limit_lower = new_lower;
    this->m_humidity_limit_upper = new_upper;
    logger.info(QString("Station's humidity limits set to %1% - %2%")
                .arg(this->m_humidity_limit_lower)
                .arg(this->m_humidity_limit_upper)
    );
}

Polar Station::sun_position(const QDateTime& time) const {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + this->m_longitude * Rad;

    Vec3D equatorial = Universe::compute_sun_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], this->m_latitude * Rad, alt, az);

    return Polar(az + pi, alt);
}

double Station::sun_altitude(const QDateTime& time) const {
    return this->sun_position(time).theta * Deg;
}

double Station::sun_azimuth(const QDateTime& time) const {
    return fmod(this->sun_position(time).phi * Deg + 360.0, 360.0);
}

// Manual and safety getters and setters
void Station::set_manual_control(bool manual) {
    this->m_manual_control = manual;
}

bool Station::is_manual(void) const { return this->m_manual_control; }

void Station::set_safety_override(bool override) {
    if (this->m_manual_control) {
        this->m_safety_override = override;
    } else {
        logger.error("Cannot override safety when not in manual mode!");
    }
}

bool Station::is_safety_overridden(void) const { return this->m_safety_override; }

QJsonObject Station::prepare_heartbeat(void) const {
     return QJsonObject {
        {"auto", !this->is_manual()},
        {"time", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"dome", this->m_dome->json()},
        {"disk", QJsonObject {
            {"prim", this->m_primary_storage->json()},
            {"perm", this->m_permanent_storage->json()},
        }}
    };
}

void Station::log_state(void) {
    const DomeStateS& stateS = this->dome()->state_S();
    const DomeStateT& stateT = this->dome()->state_T();
    const DomeStateZ& stateZ = this->dome()->state_Z();
    QString line = QString("%1 %2C %3C %4C %5% %6")
                        .arg(QString(stateS.full_text()))
                        .arg(stateT.temperature_sht(), 6, 'f', 1)
                        .arg(stateT.temperature_lens(), 6, 'f', 1)
                        .arg(stateT.temperature_cpu(), 6, 'f', 1)
                        .arg(stateT.humidity_sht(), 6, 'f', 1)
                        .arg(stateZ.shaft_position(), 4);

    this->m_state_logger->log(line);
}

// Perform automatic state checks
void Station::automatic_check(void) {
    logger.debug("Automatic check");

    const DomeStateS &stateS = this->m_dome->state_S();

    if (!stateS.is_valid()) {
        logger.warning("Automatic loop: S state is not valid, automatic loop skipped");
        return;
    }

    // Emergency: close the cover
    if (stateS.dome_open_sensor_active() && !this->is_dark()) {
        logger.warning("Closing the cover (automatic)");
        this->close_cover();
    }

    // Emergency: turn off the image intensifier
    if (stateS.intensifier_active() && !this->is_dark()) {
        logger.warning("Turning off the image intensifier (automatic)");
        this->turn_off_intensifier();
    }

    if (this->m_manual_control) {
        // If we are in manual mode, pass other automatic checks
        logger.debug("Manual control mode, passing");
    } else {
        if (this->is_dark()) {
            // If it is dark, not raning and the computer is running, start observing
            if (stateS.dome_closed_sensor_active() && !stateS.rain_sensor_active() && stateS.computer_power_sensor_active() && !this->is_humid()) {
                logger.info("Opened the cover (automatic)");
                this->open_cover();
            }

            // If humidity is very high, close the cover
            if (this->is_very_humid()) {
                logger.info("Closed the cover due to high humidity (automatic)");
                this->close_cover();
            }

            // If the dome is open, turn on the image intensifier and the fan
            if (stateS.dome_open_sensor_active()) {
                if (!stateS.intensifier_active()) {
                    logger.info("Turned on the image intensifier (automatic)");
                    this->turn_on_intensifier();
                }

                if (!stateS.fan_active()) {
                    logger.info("Turned on the fan (automatic)");
                    this->turn_on_fan();
                }
            }
        } else {
            // If it is not dark, close the cover and turn off the image intensifier
            if (stateS.dome_open_sensor_active()) {
                logger.info("Closed the cover (automatic)");
                this->close_cover();
            }
            if (stateS.intensifier_active()) {
                logger.info("Turned off the image intensifier (automatic)");
                this->turn_off_intensifier();
            }
        }
    }
}

// High level command to open the cover. Opens only if it is dark, or if in override mode.
void Station::open_cover(void) {
    if (this->is_dark() || (this->m_manual_control && this->m_safety_override)) {
        this->m_dome->send_command(Dome::CommandOpenCover);
    }
}

// High level command to close the cover. Closes anytime.
void Station::close_cover(void) {
    this->m_dome->send_command(Dome::CommandCloseCover);
}

// High level command to turn on the hotwire. Works anytime
void Station::turn_on_hotwire(void) {
    this->m_dome->send_command(Dome::CommandHotwireOn);
}

// High level command to turn off the hotwire. Works anytime
void Station::turn_off_hotwire(void) {
    this->m_dome->send_command(Dome::CommandHotwireOff);
}

// High level command to turn on the fan. Works anytime
void Station::turn_on_fan(void) {
    this->m_dome->send_command(Dome::CommandFanOn);
}

// High level command to turn off the fan. Works anytime
void Station::turn_off_fan(void) {
    this->m_dome->send_command(Dome::CommandFanOff);
}

// High level command to turn on the intensifier. Turns on only if it is dark, or if in override mode.
void Station::turn_on_intensifier(void) {
    if (this->is_dark() || (this->m_manual_control && this->m_safety_override)) {
        this->m_dome->send_command(Dome::CommandIIOn);
    } else {
        logger.warning("Command ignored, sun is too high and override is not active");
    }
}

// High level command to turn off the intensifier. Works anytime
void Station::turn_off_intensifier(void) {
    this->m_dome->send_command(Dome::CommandIIOff);
}

StationState Station::determine_state(void) const {
    if (this->is_manual()) {
        return StationState::MANUAL;
    }

    if (this->m_dome->state_S().is_valid()) {
        if (this->is_dark()) {
            if (this->m_dome->state_S().dome_open_sensor_active() && this->m_dome->state_S().intensifier_active()) {
                return StationState::OBSERVING;
            } else {
                return StationState::NOT_OBSERVING;
            }
        } else {
            return StationState::DAY;
        }
    } else {
        return StationState::DOME_UNREACHABLE;
    }
}

void Station::send_heartbeat(void) {
    this->m_server->send_heartbeat(this->prepare_heartbeat());
}

void Station::check_state(void) {
    StationState new_state = this->determine_state();

    if (this->m_state != new_state) {
        this->m_state = new_state;
        emit this->state_changed();
    }
}

StationState Station::state(void) {
    this->m_state = this->determine_state();
    return this->m_state;
}

void Station::file_check(void) {
    logger.debug("Checking files...");
    for (auto sighting: this->primary_storage().list_new_sightings()) {
        this->send_sighting(sighting);
        this->move_sighting(sighting);
    }
}

void Station::send_sighting(const Sighting &sighting) {
    this->m_server->send_sighting(sighting);
}

void Station::move_sighting(Sighting &sighting) {
    this->m_primary_storage->move_sighting(sighting);
}
