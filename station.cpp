#include "include.h"

Station::Station(const QString& _id, const QDir& primary_storage_dir, const QDir& permanent_storage_dir) {
    this->id = _id;

    this->primary_storage = new Storage("primary", primary_storage_dir);
    this->permanent_storage = new Storage("permanent", permanent_storage_dir);
}

Polar Station::sun_position(const QDateTime& time) {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + this->longitude * Rad;

    Vec3D equatorial = Universe::compute_sun_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], this->latitude * Rad, alt, az);

    return Polar(az + pi, alt);
}

double Station::get_sun_altitude(void) {
    return this->sun_position().theta * Deg;
}

double Station::get_sun_azimuth(void) {
    return fmod(this->sun_position().phi * Deg + 360.0, 360.0);
}

Storage& Station::get_primary_storage(void) {
    return *this->primary_storage;
}

Storage& Station::get_permanent_storage(void) {
    return *this->permanent_storage;
}

QJsonObject Station::prepare_heartbeat(void) const {
    QJsonObject json {
        {"automatic", this->automatic},
        {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"dome", this->dome_manager.json()},

        {"disk", QJsonObject {
            {"primary", this->primary_storage->json()},
            {"permanent", this->permanent_storage->json()},
        }}
    };

    return QJsonObject(json);
}
