#include "station.h"
#include "universe.h"

Station::Station() {

}

Polar Station::sun_position(const QDateTime& time) {
    double alt, az, ra, de;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + this->longitude * Rad;

    MiniSun((mjd + Universe::delta_t - MJD_J2000) / 36525.0, ra, de);
    Equ2Hor(de, lmst - ra, this->latitude * Rad, alt, az);

    return Polar(az + pi, alt);
}

double Station::get_sun_altitude(void) {
    this->sun_altitude = this->sun_position().theta * Deg;
    return this->sun_altitude;
}

double Station::get_sun_azimuth(void) {
    this->sun_azimuth = this->sun_position().phi * Deg;
    return this->sun_azimuth;
}

QJsonObject Station::prepare_heartbeat(void) const {
    QJsonObject json;

    json["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    json["dome"] = this->dome_manager.json();
    json["automatic"] = this->automatic;

    return QJsonObject(json);
}
