#include "include.h"
#include <windows.h>
#include <winuser.h>

extern EventLogger logger;

const StationState Station::Daylight            = StationState('D', "daylight", QIcon(":/images/yellow.ico"), "not observing, daylight");
const StationState Station::Observing           = StationState('O', "observing", QIcon(":/images/blue.ico"), "observation in progress");
const StationState Station::NotObserving        = StationState('N', "not observing", QIcon(":/images/grey.ico"), "observation stopped");
const StationState Station::Manual              = StationState('M', "manual", QIcon(":/images/green.ico"), "manual control enabled");
const StationState Station::DomeUnreachable     = StationState('U', "dome unreachable", QIcon(":/images/red.ico"), "serial port has no connection");
const StationState Station::Raining             = StationState('R', "raining", QIcon(":/images/grey.ico"), "rain sensor active");
const StationState Station::Humid               = StationState('H', "humid", QIcon(":/images/grey.ico"), "humidity limit reached");
const StationState Station::NoMasterPower       = StationState('P', "no master power", QIcon(":/images/grey.ico"), "master power sensor inactive");


QString Station::temperature_colour(float temperature) {
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

Station::Station(const QString& id):
    m_id(id),
    m_manual_control(false),
    m_safety_override(false),
    m_state(Station::NotObserving),
    m_filesystemscanner(nullptr),
    m_server(nullptr)
{
    this->m_dome = new Dome();
    this->m_state_logger = new StateLogger(this, "state.log");
    this->m_state_logger->initialize();

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
    delete this->m_filesystemscanner;
    delete this->m_primary_storage;
    delete this->m_permanent_storage;
    delete this->m_ufo_manager;
    delete this->m_server;
}

void Station::set_scanner(const QDir &directory) {
    delete this->m_filesystemscanner;
    this->m_filesystemscanner = new FileSystemScanner(directory);
    this->connect(this->m_filesystemscanner, &FileSystemScanner::sightings_found, this, &Station::process_sightings);
}

FileSystemScanner* Station::scanner(void) const { return this->m_filesystemscanner; }

void Station::set_storages(const QDir &primary_storage_dir, const QDir &permanent_storage_dir) {
    this->m_primary_storage = new Storage("primary", primary_storage_dir);
    this->m_permanent_storage = new Storage("permanent", permanent_storage_dir);
}

void Station::set_ufo_manager(UfoManager *ufo_manager) { this->m_ufo_manager = ufo_manager; }
UfoManager* Station::ufo_manager(void) const { return this->m_ufo_manager; }

Storage* Station::primary_storage(void) const { return this->m_primary_storage; }
Storage* Station::permanent_storage(void) const { return this->m_permanent_storage; }
Dome* Station::dome(void) const { return this->m_dome; }

// Server getters and setters
void Station::set_server(Server *server) { this->m_server = server; }
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
    logger.info(Concern::Configuration, QString("Station position set to %1°, %2°, %3 m").arg(this->m_latitude).arg(this->m_longitude).arg(this->m_altitude));

    emit this->position_changed();
}

double Station::latitude(void) const { return this->m_latitude; }
double Station::longitude(void) const { return this->m_longitude; }
double Station::altitude(void) const { return this->m_altitude; }

void Station::set_id(const QString &new_id) {
    if (new_id.length() > 4) {
        throw ConfigurationError(QString("Cannot set station id to '%1'").arg(new_id));
    }

    this->m_id = new_id;
    logger.info(Concern::Configuration, QString("Station id set to '%1'").arg(this->m_id));

    emit this->id_changed();
}

const QString& Station::get_id(void) const { return this->m_id; }

// Darkness limit settings
bool Station::is_dark(const QDateTime &time) const {
    return (this->sun_altitude(time) < this->m_darkness_limit);
}

double Station::darkness_limit(void) const { return this->m_darkness_limit; }

void Station::set_darkness_limit(const double new_darkness_limit) {
    if ((new_darkness_limit < -18) || (new_darkness_limit > 0)) {
        throw ConfigurationError(QString("Darkness limit out of range: %1°").arg(new_darkness_limit));
    }

    this->m_darkness_limit = new_darkness_limit;
    logger.info(Concern::Configuration, QString("Station's darkness limit set to %1°").arg(this->m_darkness_limit));

    emit this->darkness_limit_changed(new_darkness_limit);
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
    if ((new_lower < 0) || (new_lower > 100) || (new_upper < 0) || (new_upper > 100) || (new_lower > new_upper)) {
        throw ConfigurationError(QString("Invalid humidity limits: %1% - %2%").arg(new_lower).arg(new_upper));
    }

    this->m_humidity_limit_lower = new_lower;
    this->m_humidity_limit_upper = new_upper;
    logger.info(Concern::Configuration,
                QString("Station's humidity limits set to %1% - %2%")
                .arg(this->m_humidity_limit_lower)
                .arg(this->m_humidity_limit_upper)
    );

    emit this->humidity_limits_changed(new_lower, new_upper);
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

/* Compute next sun crossing of almucantar `altitude` in the specified direction
 * with resolution of `resolution` seconds (optional, default = 60) */
QDateTime Station::next_sun_crossing(double altitude, bool direction_up, int resolution) const {
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

// Manual and safety getters and setters
void Station::set_manual_control(bool manual) {
    this->m_manual_control = manual;
    this->automatic_check();
}

bool Station::is_manual(void) const { return this->m_manual_control; }

void Station::set_safety_override(bool override) {
    if (this->m_manual_control) {
        this->m_safety_override = override;
    } else {
        logger.error(Concern::SerialPort, "Cannot override safety when not in manual mode!");
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
        }},
        {"cfg", QJsonObject {
            {"dl", this->m_darkness_limit},
            {"hll", this->m_humidity_limit_lower},
            {"hlu", this->m_humidity_limit_upper},
        }},
    };
}

void Station::log_state(void) {
    const DomeStateS& stateS = this->dome()->state_S();
    const DomeStateT& stateT = this->dome()->state_T();
    const DomeStateZ& stateZ = this->dome()->state_Z();
    QString line = QString("%1 %2° %3C %4C %5C %6% %7")
                        .arg(QString(stateS.full_text()))
                        .arg(this->sun_altitude(), 5, 'f', 1)
                        .arg(stateT.temperature_sht(), 5, 'f', 1)
                        .arg(stateT.temperature_lens(), 5, 'f', 1)
                        .arg(stateT.temperature_cpu(), 5, 'f', 1)
                        .arg(stateT.humidity_sht(), 5, 'f', 1)
                        .arg(stateZ.shaft_position(), 3);

    this->m_state_logger->log(line);
}

// Perform automatic state checks
void Station::automatic_check(void) {
    const DomeStateS &stateS = this->m_dome->state_S();

    if (!stateS.is_valid()) {
        logger.debug_error(Concern::Automatic, "S state is not valid, automatic loop skipped");
        this->set_state(Station::DomeUnreachable);
        return;
    }

    // Emergency: close the cover, unless safety overridden
    if (stateS.dome_open_sensor_active() && !this->is_dark()) {
        if (this->m_safety_override) {
            logger.warning(Concern::Automatic, "Emergency override active, not closing");
        } else {
            logger.warning(Concern::Automatic, "Closed the cover (not dark enough)");
            this->close_cover();
        }
    }

    // Emergency: turn off the image intensifier, unless safety overridden
    if (stateS.intensifier_active() && !this->is_dark()) {
        if (this->m_safety_override) {
            logger.warning(Concern::Automatic, "Emergency override active, not turning off the intensifier");
        } else {
            logger.warning(Concern::Automatic, "Turned off the image intensifier (not dark enough)");
            this->turn_off_intensifier();
        }
    }

    if (this->m_manual_control) {
        // If we are in manual mode, pass other automatic checks
        logger.debug(Concern::Automatic, "Manual control mode, passing");
        this->set_state(Station::Manual);
    } else {
        if (this->is_dark()) {
            if (stateS.dome_closed_sensor_active()) {
                if (stateS.rain_sensor_active()) {
                    logger.debug(Concern::Automatic, "Will not open: raining");
                    this->set_state(Station::Raining);
                } else {
                    if (stateS.computer_power_sensor_active()) {
                        if (this->is_humid()) {
                            logger.debug(Concern::Automatic, "Will not open: humidity is too high");
                            this->set_state(Station::Humid);
                        } else {
                            logger.info(Concern::Automatic, "Opening the cover");
                            this->open_cover();
                        }
                    } else {
                        logger.debug(Concern::Automatic, "Will not open: master power off");
                        this->set_state(Station::NoMasterPower);
                    }
                }

                // If the dome is closed, also turn off the intensifier
                if (stateS.intensifier_active()) {
                    logger.info(Concern::Automatic, "Cover is closed, turned off the intensifier");
                    this->turn_off_intensifier();
                }
            } else {
                logger.debug(Concern::Automatic, "Will not open: daylight");
            }

            if (stateS.dome_open_sensor_active()) {
                // If the dome is open, turn on the image intensifier and the fan
                if (stateS.intensifier_active()) {
                    logger.debug(Concern::Automatic, "Intensifier active");
                    this->set_state(Station::Observing);
                } else {
                    logger.info(Concern::Automatic, "Cover open, turned on the image intensifier");
                    this->turn_on_intensifier();
                }

                if (!stateS.fan_active()) {
                    logger.info(Concern::Automatic, "Turned on the fan");
                    this->turn_on_fan();
                }

                // But if humidity is very high, close the cover
                if (this->is_very_humid()) {
                    logger.info(Concern::Automatic, "Closed the cover (high humidity)");
                    this->close_cover();
                }
            } else {
                this->set_state(Station::NotObserving);
            }
        } else {
            logger.debug(Concern::Automatic, "Will not open: daylight");
            this->set_state(Station::Daylight);
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
        logger.warning(Concern::SerialPort, "Command ignored, sun is too high and override is not active");
    }
}

// High level command to turn off the intensifier. Works anytime
void Station::turn_off_intensifier(void) {
    this->m_dome->send_command(Dome::CommandIIOff);
}

void Station::set_state(StationState new_state) {
    logger.info(Concern::Operation, QString("State set to %1 %2").arg(QString(new_state.code()), new_state.display_string()));
  //  if (new_state != this->m_state) {
        logger.info(Concern::Operation, QString("State changed to %1").arg(new_state.display_string()));
        this->m_state = new_state;
        emit this->state_changed(this->m_state);
    //}
}

void Station::send_heartbeat(void) {
    this->m_server->send_heartbeat(this->prepare_heartbeat());
}

StationState Station::state(void) {
    return this->m_state;
}

void Station::file_check(void) {
    logger.debug(Concern::Automatic, "Checking files...");

    this->m_ufo_manager->auto_action(this->is_dark());
}

void Station::process_sightings(QVector<Sighting> sightings) {
    for (auto &sighting: sightings) {
        this->m_server->send_sighting(sighting);
        this->m_permanent_storage->store_sighting(sighting, false);
        this->m_primary_storage->store_sighting(sighting, true);
    }
}
