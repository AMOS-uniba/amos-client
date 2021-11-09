#include "universe.h"

// Compute modified Julian date for specified UTC time
double Universe::mjd(const QDateTime & time) {
    return 40587.0 + ((double) time.toSecsSinceEpoch()) / 86400.0;
}

// Compute Julian centuries since J2000.0
double Universe::julian_centuries(const QDateTime & time) {
    return (Universe::mjd(time) + Universe::delta_t - MJD_J2000) / 36525.0;
}

// Compute ecliptical coordinates of the Sun
Vec3D Universe::compute_sun_ecl(const QDateTime & time) {
    return SunPos(Universe::julian_centuries(time));
}

// Compute equatorial coordinates of the Sun
Vec3D Universe::compute_sun_equ(const QDateTime & time) {
    double ra, dec;
    MiniSun(Universe::julian_centuries(time), ra, dec);
    return Vec3D(Polar(ra, dec));
}

Vec3D Universe::compute_moon_equ(const QDateTime & time) {
    double ra, dec;
    MiniMoon(Universe::julian_centuries(time), ra, dec);
    return Vec3D(Polar(ra, dec));
}

/** Sun functions **/
Polar Universe::sun_position(const double latitude, const double longitude, const QDateTime & time) {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + longitude * Rad;

    Vec3D equatorial = Universe::compute_sun_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], latitude * Rad, alt, az);

    return Polar(fmod(az + pi, 2 * pi), alt);
}

Polar Universe::moon_position(const double latitude, const double longitude, const QDateTime & time) {
    double alt, az;
    double mjd = Universe::mjd(time);
    double lmst = GMST(mjd) + longitude * Rad;

    Vec3D equatorial = Universe::compute_moon_equ(time);
    Equ2Hor(equatorial[theta], lmst - equatorial[phi], latitude * Rad, alt, az);

    return Polar(fmod(az + pi, 2 * pi), alt);
}

double Universe::sun_altitude(const double latitude, const double longitude, const QDateTime & time) {
    return Universe::sun_position(latitude, longitude, time).theta * Deg;
}

double Universe::sun_azimuth(const double latitude, const double longitude, const QDateTime & time) {
    return fmod(Universe::sun_position(latitude, longitude, time).phi * Deg + 360.0, 360.0);
}

double Universe::moon_altitude(const double latitude, const double longitude, const QDateTime & time) {
    return Universe::moon_position(latitude, longitude, time).theta * Deg;
}

double Universe::moon_azimuth(const double latitude, const double longitude, const QDateTime & time) {
    return fmod(Universe::moon_position(latitude, longitude, time).phi * Deg + 360.0, 360.0);
}

QDateTime Universe::next_crossing(std::function<double(double, double, QDateTime)> fun,
                                  double latitude, double longitude, double altitude, bool direction_up, int resolution) {
    QDateTime now = QDateTime::fromTime_t((QDateTime::currentDateTimeUtc().toTime_t() / resolution) * resolution);
    double oldalt = fun(latitude, longitude, now);

    for (int i = 1; i <= 86400 / resolution; ++i) {
        QDateTime instant = now.addSecs(resolution * i);
        double newalt = fun(latitude, longitude, instant);
        if (direction_up) {
            if ((oldalt < altitude) && (newalt > altitude)) {
                return instant;
            }
        } else {
            if ((oldalt > altitude) && (newalt < altitude)) {
                return instant;
            }
        }
        oldalt = newalt;
    }
    return QDateTime();
}

