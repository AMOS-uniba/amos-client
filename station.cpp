#include "include.h"

extern Log logger;

Station::Station(const QString& id): m_id(id) {
    this->dome = new Dome();
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

void Station::check_sun(void) {
/*    if (!this->manual) {
        if (this->is_dark()) {
            this->dome->open_cover();
        } else {
            this->dome->close_cover();
        }
    } */
}

QJsonObject Station::prepare_heartbeat(void) const {
     return QJsonObject {
        {"auto", !this->manual},
        {"time", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"dome", this->dome->json()},
        {"disk", QJsonObject {
            {"prim", this->m_primary_storage->json()},
            {"perm", this->m_permanent_storage->json()},
        }}
    };
}
