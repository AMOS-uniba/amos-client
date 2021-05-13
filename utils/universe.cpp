#include "include.h"

Universe::Universe() {

}

// Compute modified Julian date for specified UTC time
double Universe::mjd(const QDateTime &time) {
    return 40587.0 + ((double) time.toSecsSinceEpoch()) / 86400.0;
}

// Compute Julian centuries since J2000.0
double Universe::julian_centuries(const QDateTime &time) {
    return (Universe::mjd(time) + Universe::delta_t - MJD_J2000) / 36525.0;
}

// Compute ecliptical coordinates of the Sun
Vec3D Universe::compute_sun_ecl(const QDateTime &time) {
    return SunPos(Universe::julian_centuries(time));
}

// Compute equatorial coordinates of the Sun
Vec3D Universe::compute_sun_equ(const QDateTime &time) {
    double ra, dec;
    MiniSun(Universe::julian_centuries(time), ra, dec);
    return Vec3D(Polar(ra, dec));
}

Vec3D Universe::compute_moon_equ(const QDateTime &time) {
    double ra, dec;
    MiniMoon(Universe::julian_centuries(time), ra, dec);
    return Vec3D(Polar(ra, dec));
}

QColor linear_interpolate(QColor first, double stop1, QColor second, double stop2, double value) {
    double x = (value - stop1) / (stop2 - stop1);
    return QColor::fromRgbF(
        first.redF() * x + second.redF() * (1 - x),
        first.greenF() * x + second.greenF() * (1 - x),
        first.blueF() * x + second.blueF() * (1 - x)
    );
}

QColor Universe::altitude_colour(double altitude) {
    if (altitude < -18) {
        return linear_interpolate(QColor::fromHslF(0, 0, 0), -90, QColor::fromRgbF(0, 0, 0.25), -18, altitude);
    }

    if (altitude < 0) {
        return linear_interpolate(QColor::fromHslF(0, 0, 0.25), -18, QColor::fromRgbF(0, 0, 1), 0, altitude);
    }

    if (altitude < 6) {
        return linear_interpolate(QColor::fromHslF(1, 0.25, 0), 0, QColor::fromRgbF(1, 0.75, 0), 6, altitude);
    }

    if (altitude < 18) {
        return linear_interpolate(QColor::fromHslF(1, 0.75, 0), 6, QColor::fromRgbF(0, 0.625, 0.875), 18, altitude);
    }

    return linear_interpolate(QColor::fromHslF(0, 0.625, 0.875), 18, QColor::fromRgbF(0, 0.875, 1), 90, altitude);
}
