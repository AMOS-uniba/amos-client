#ifndef UNIVERSE_H
#define UNIVERSE_H

#include "station.h"

#include "APC\APC_include.h"

class Universe {
private:
    double sun_ecl_latitude;
    double sun_ecl_longitude;
public:
    constexpr static double delta_t = 67.28 / 86400.0;

    Universe();

    static Vec3D compute_sun_ecl(const QDateTime& time = QDateTime::currentDateTimeUtc());
    static Vec3D compute_sun_equ(const QDateTime& time = QDateTime::currentDateTimeUtc());

    static double mjd(const QDateTime& time = QDateTime::currentDateTimeUtc());
    static double julian_centuries(const QDateTime& time = QDateTime::currentDateTimeUtc());

    double get_sun_latitude(void) const;
    double get_sun_longitude(void) const;

    double get_sun_altitude(const Station& station) const;
    double get_sun_azimuth(const Station& station) const;

    void compute_sun(const QDateTime& time) const;
};

#endif // UNIVERSE_H
