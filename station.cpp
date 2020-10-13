#include "include.h"

extern Log logger;

Station::Station(const QString& _id, const QDir& primary_storage_dir, const QDir& permanent_storage_dir) {
    this->id = _id;

    this->primary_storage = new Storage("primary", primary_storage_dir);
    this->permanent_storage = new Storage("permanent", permanent_storage_dir);

    this->dome_manager = new DomeManager();
}

Station::~Station(void) {
    delete this->dome_manager;
    delete this->primary_storage;
    delete this->permanent_storage;
}

void Station::set_position(const double new_latitude, const double new_longitude, const double new_altitude) {
    if (fabs(new_latitude) > 90) {
        throw ConfigurationError(QString("Latitude out of range: %1").arg(new_latitude));
    }
    if (fabs(new_longitude) > 180) {
        throw ConfigurationError(QString("Longitude out of range: %1").arg(new_longitude));
    }

    this->latitude = new_latitude;
    this->longitude = new_longitude;
    this->altitude = new_altitude;
    logger.info(QString("Station position set to %1°, %2°, %3 m").arg(this->latitude).arg(this->longitude).arg(this->altitude));
}

void Station::set_id(const QString& new_id) {
    if (new_id.length() > 4) {
        throw ConfigurationError(QString("Cannot set station id to '%1'").arg(new_id));
    }

    this->id = new_id;
    logger.info(QString("Station id changed to '%1'").arg(this->id));
}

const QString& Station::get_id(void) const {
    return this->id;
}

bool Station::is_dark(const QDateTime& time) const {
    return (this->get_sun_altitude(time) < this->altitude_dark);
}

void Station::set_altitude_dark(const double new_altitude_dark) {
    if ((new_altitude_dark < -18) || (new_altitude_dark > 1)) {
        throw ConfigurationError(QString("Darkness limit out of range: %1°").arg(new_altitude_dark));
    }

    this->altitude_dark = new_altitude_dark;
    logger.info(QString("Darkness limit set to %1°").arg(new_altitude_dark));
}

Polar Station::sun_position(const QDateTime& time) const {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + this->longitude * Rad;

    Vec3D equatorial = Universe::compute_sun_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], this->latitude * Rad, alt, az);

    return Polar(az + pi, alt);
}

double Station::get_sun_altitude(const QDateTime& time) const {
    return this->sun_position(time).theta * Deg;
}

double Station::get_sun_azimuth(const QDateTime& time) const {
    return fmod(this->sun_position(time).phi * Deg + 360.0, 360.0);
}

Storage& Station::get_primary_storage(void) {
    return *this->primary_storage;
}

Storage& Station::get_permanent_storage(void) {
    return *this->permanent_storage;
}

void Station::check_sun(void) {
    logger.debug("Checking the Sun's altitude");
    if (this->automatic) {
        if (this->is_dark()) {
            this->dome_manager->open_cover();
        } else {
            this->dome_manager->close_cover();
        }
    }
}

QJsonObject Station::prepare_heartbeat(void) const {
     return QJsonObject {
        {"auto", this->automatic},
        {"time", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"dome", this->dome_manager->json()},

        {"disk", QJsonObject {
            {"prim", this->primary_storage->json()},
            {"perm", this->permanent_storage->json()},
        }}
    };
}
