#include "include.h"

extern Log logger;

Station::Station(const QString& id): m_id(id) {
    this->dome = new Dome();

    this->m_timer_automatic = new QTimer(this);
    this->m_timer_automatic->setInterval(1000);
    this->connect(this->m_timer_automatic, &QTimer::timeout, this, &Station::automatic_check);
}

Station::~Station(void) {
    delete this->dome;
    delete this->m_primary_storage;
    delete this->m_permanent_storage;
}

void Station::set_storages(const QDir& primary_storage_dir, const QDir& permanent_storage_dir) {
    this->m_primary_storage = new Storage("primary", primary_storage_dir);
    this->m_permanent_storage = new Storage("permanent", permanent_storage_dir);
}

Storage& Station::primary_storage(void) {
    return *this->m_primary_storage;
}

Storage& Station::permanent_storage(void) {
    return *this->m_permanent_storage;
}

// Position getters and setters
void Station::set_position(const double new_latitude, const double new_longitude, const double new_altitude) {
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
    logger.info(QString("Station id changed to '%1'").arg(this->m_id));
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
    logger.info(QString("Darkness limit set to %1°").arg(m_darkness_limit));
}

// Humidity limit settings
double Station::humidity_limit(void) const { return this->m_humidity_limit; }

void Station::set_humidity_limit(const double new_humidity_limit) {
    if ((new_humidity_limit < 0) || (new_humidity_limit > 100)) {
        throw ConfigurationError(QString("Darkness limit out of range: %1°").arg(m_humidity_limit));
    }

    this->m_humidity_limit = new_humidity_limit;
    logger.info(QString("Humidity limit set to %1%").arg(m_humidity_limit));
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
        {"dome", this->dome->json()},
        {"disk", QJsonObject {
            {"prim", this->m_primary_storage->json()},
            {"perm", this->m_permanent_storage->json()},
        }}
    };
}

void Station::automatic_check(void) {
    logger.debug("Automatic check");
    const DomeStateS &state = this->dome->state_S();

    if (state.is_valid() && state.dome_open_sensor_active() && !this->is_dark()) {
        logger.info("Closing the cover (automatic)");
        this->close_cover();
    }

    if (state.is_valid() && state.intensifier_active() && !this->is_dark()) {
        logger.info("Turning off the image intensifier (automatic))");
        this->turn_off_intensifier();
    }


    // should only do this in automatic mode
    if (this->is_dark()) {
        if (state.dome_closed_sensor_active() && !state.rain_sensor_active() && state.computer_power_sensor_active()) {
            logger.info("Opening the cover (automatic)");
            this->open_cover();
        }

        if (state.dome_open_sensor_active()) {
            if (!state.intensifier_active()) {
                logger.info("Turning on the image intensifier (automatic)");
                this->turn_on_intensifier();
            }

            if (!state.fan_active()) {
                logger.info("Turning on the fan (automatic)");
                this->turn_on_fan();
            }
        }
    } else {
        if (state.dome_open_sensor_active()) {
            logger.info("Closing the cover (automatic)");
            this->close_cover();
        }
        if (state.intensifier_active()) {
            logger.info("Turning off the image intensifier (automatic))");
            this->turn_off_intensifier();
        }
    }
}

// High level command to open the cover. Opens only if it is dark, or if in override mode.
void Station::open_cover(void) {
    if (this->is_dark() || (this->m_manual_control && this->m_safety_override)) {
        this->dome->send_command(Dome::CommandOpenCover);
    }
}

// High level command to close the cover. Closes anytime.
void Station::close_cover(void) {
    this->dome->send_command(Dome::CommandCloseCover);
}

// High level command to turn on the hotwire. Works anytime
void Station::turn_on_hotwire(void) {
    this->dome->send_command(Dome::CommandHotwireOn);
}

// High level command to turn off the hotwire. Works anytime
void Station::turn_off_hotwire(void) {
    this->dome->send_command(Dome::CommandHotwireOff);
}

// High level command to turn on the fan. Works anytime
void Station::turn_on_fan(void) {
    this->dome->send_command(Dome::CommandFanOn);
}

// High level command to turn off the fan. Works anytime
void Station::turn_off_fan(void) {
    this->dome->send_command(Dome::CommandFanOff);
}

// High level command to turn on the intensifier. Turns on only if it is dark, or if in override mode.
void Station::turn_on_intensifier(void) {
    if (this->is_dark() || (this->m_manual_control && this->m_safety_override)) {
        this->dome->send_command(Dome::CommandIIOn);
    } else {
        logger.warning("Command ignored, sun is too high and override is not active");
    }
}

// High level command to turn off the intensifier. Works anytime
void Station::turn_off_intensifier(void) {
    this->dome->send_command(Dome::CommandIIOff);
}
