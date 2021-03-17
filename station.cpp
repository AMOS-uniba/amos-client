#include "include.h"
#include <windows.h>
#include <winuser.h>

extern EventLogger logger;

const StationState Station::Daylight            = StationState('D', "daylight", Icon::Daylight, "not observing, daylight");
const StationState Station::Observing           = StationState('O', "observing", Icon::Observing, "observation in progress");
const StationState Station::NotObserving        = StationState('N', "not observing", Icon::NotObserving, "observation stopped");
const StationState Station::Manual              = StationState('M', "manual", Icon::Manual, "manual control enabled");
const StationState Station::DomeUnreachable     = StationState('U', "dome unreachable", Icon::Failure, "serial port has no connection");
const StationState Station::RainOrHumid         = StationState('R', "rain or high humidity", Icon::NotObserving, "rain sensor active or humidity too high");
const StationState Station::NoMasterPower       = StationState('P', "no master power", Icon::NotObserving, "master power sensor inactive");

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
    m_scanner(nullptr),
    m_primary_storage(nullptr),
    m_permanent_storage(nullptr),
    m_server(nullptr)
{
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
    delete this->m_timer_automatic;
    delete this->m_ufo_manager;
    delete this->m_server;
}

void Station::set_storages(QStorageBox *primary_storage, QStorageBox *permanent_storage) {
    this->m_primary_storage = primary_storage;
    this->m_permanent_storage = permanent_storage;
}

void Station::set_ufo_manager(UfoManager *ufo_manager) { this->m_ufo_manager = ufo_manager; }
UfoManager* Station::ufo_manager(void) const { return this->m_ufo_manager; }

QStorageBox* Station::primary_storage(void) const { return this->m_primary_storage; }
QStorageBox* Station::permanent_storage(void) const { return this->m_permanent_storage; }

void Station::set_dome(QDome *dome) { this->m_dome = dome; }
QDome* Station::dome(void) const { return this->m_dome; }

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

Polar Station::sun_position(const QDateTime &time) const {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + this->m_longitude * Rad;

    Vec3D equatorial = Universe::compute_sun_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], this->m_latitude * Rad, alt, az);

    return Polar(az + pi, alt);
}

Polar Station::moon_position(const QDateTime &time) const {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + this->m_longitude * Rad;

    Vec3D equatorial = Universe::compute_moon_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], this->m_latitude * Rad, alt, az);

    return Polar(fmod(az + pi, 2 * pi), alt);
}

double Station::sun_altitude(const QDateTime &time) const {
    return this->sun_position(time).theta * Deg;
}

double Station::sun_azimuth(const QDateTime &time) const {
    return fmod(this->sun_position(time).phi * Deg + 360.0, 360.0);
}

double Station::moon_altitude(const QDateTime &time) const {
    return this->moon_position(time).theta * Deg;
}

double Station::moon_azimuth(const QDateTime &time) const {
    return fmod(this->moon_position(time).phi * Deg + 360.0, 360.0);
}

/* Compute next crossing of the Sun through the almucantar `altitude` in the specified direction (up/down)
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
        logger.error(Concern::Operation, "Cannot override safety when not in manual mode!");
    }
}

bool Station::is_safety_overridden(void) const { return this->m_safety_override; }

QJsonObject Station::prepare_heartbeat(void) const {
     return QJsonObject {
        {"auto", !this->is_manual()},
        {"time", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"dome", this->m_dome->json()},
        {"st", QString(this->m_state.code())},
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
    QString line = QString("%1° %2 %3")
                        .arg(this->sun_altitude(), 5, 'f', 1)
                        .arg(QString(this->state().code()))
                        .arg(this->m_dome->status_line());
    this->m_state_logger->log(line);
}

QString Station::state_logger_filename(void) const {
    return this->m_state_logger->filename();
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
            logger.debug(Concern::Automatic, "Emergency override active, not closing");
        } else {
            logger.warning(Concern::Automatic, "Closed the cover (not dark enough)");
            this->dome()->close_cover();
        }
    }

    // Emergency: turn off the image intensifier, unless safety overridden
    if (stateS.intensifier_active() && !this->is_dark()) {
        if (this->m_safety_override) {
            logger.debug(Concern::Automatic, "Emergency override active, not turning off the intensifier");
        } else {
            logger.warning(Concern::Automatic, "Turned off the image intensifier (not dark enough)");
            this->dome()->turn_off_intensifier();
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
                    this->set_state(Station::RainOrHumid);
                } else {
                    if (this->dome()->is_humid()) {
                        logger.debug(Concern::Automatic, "Will not open: humidity is too high");
                        this->set_state(Station::RainOrHumid);
                    } else {
                        logger.info(Concern::Automatic, "Opening the cover");
                        this->m_dome->open_cover();
                    }
                }

                // If the dome is closed, also turn off the intensifier
                if (stateS.intensifier_active()) {
                    logger.info(Concern::Automatic, "Cover is closed, turned off the intensifier");
                    this->m_dome->turn_off_intensifier();
                    this->set_state(Station::NotObserving);
                }
            } else {
                if (stateS.dome_open_sensor_active()) {
                    // If the dome is open, turn on the image intensifier and the fan
                    if (stateS.intensifier_active()) {
                        logger.detail(Concern::Automatic, "Intensifier active");
                        this->set_state(Station::Observing);
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
                        this->set_state(Station::RainOrHumid);
                        this->m_dome->close_cover();
                    }
                } else {
                    this->set_state(Station::NotObserving);
                }
            }
        } else {
            logger.debug(Concern::Automatic, "Will not open: daylight");
            this->set_state(Station::Daylight);
        }
    }
}

void Station::set_state(StationState new_state) {
    if (new_state != this->m_state) {
        logger.debug(Concern::Operation, QString("State changed from %1 to %2").arg(this->state().display_string(), new_state.display_string()));
        this->m_state = new_state;
        emit this->state_changed(this->m_state);
    }
}

void Station::send_heartbeat(void) {
    this->m_server->send_heartbeat(this->prepare_heartbeat());
}

StationState Station::state(void) {
    return this->m_state;
}

void Station::file_check(void) {
    this->m_ufo_manager->auto_action(this->is_dark());
}

void Station::process_sightings(QVector<Sighting> sightings) {
    logger.debug(Concern::Sightings, "Processing sightings...");
    for (auto &sighting: sightings) {
        this->m_server->send_sighting(sighting);
        this->m_permanent_storage->store_sighting(sighting, false);
        this->m_primary_storage->store_sighting(sighting, true);
    }
}
