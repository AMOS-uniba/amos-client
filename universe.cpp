#include "include.h"

Universe::Universe() {

}

// Compute modified Julian date for specified UTC time
double Universe::mjd(const QDateTime& time) {
    return 40587.0 + ((double) time.toSecsSinceEpoch()) / 86400.0;
}

// Compute Julian centuries since J2000.0
double Universe::julian_centuries(const QDateTime& time) {
    return (Universe::mjd(time) + Universe::delta_t - MJD_J2000) / 36525.0;
}

// Compute ecliptical coordinates of the Sun
Vec3D Universe::compute_sun_ecl(const QDateTime& time) {
    return SunPos(Universe::julian_centuries(time));
}

// Compute equatorial coordinates of the Sun
Vec3D Universe::compute_sun_equ(const QDateTime& time) {
    double ra, dec;
    MiniSun(Universe::julian_centuries(time), ra, dec);
    return Vec3D(Polar(ra, dec));
}

QString Universe::altitude_colour(double altitude) {
    if (altitude < 0) {
        return QString("hsv(240, 50%, %1%)").arg(altitude * 50.0 / 90.0 + 50.0);
    } else {
        if (altitude < 30) {
            return QString("hsv(%1, 100%, 70%)").arg(altitude + 30.0);
        } else {
            return QString("hsv(60, 100%, %1%)").arg(70.0 + (20.0 * altitude - 30.0) / 60);
        }
    }
}
