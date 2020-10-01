#include "include.h"

Station::Station(const QString& _id) {
    this->id = _id;
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
    this->sun_altitude = this->sun_position().theta * Deg;
    return this->sun_altitude;
}

double Station::get_sun_azimuth(void) {
    this->sun_azimuth = fmod(this->sun_position().phi * Deg + 360.0, 360.0);
    return this->sun_azimuth;
}

QJsonObject Station::prepare_heartbeat(void) const {
    QJsonObject json;

    json["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    json["dome"] = this->dome_manager.json();
    json["automatic"] = this->automatic;

    return QJsonObject(json);
}
