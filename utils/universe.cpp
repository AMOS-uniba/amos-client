#include "universe.h"

struct Node {
    double position;
    QColor colour;
};

QColor linear_interpolate(QColor first, double stop1, QColor second, double stop2, double value) {
    double x = (value - stop1) / (stop2 - stop1);
    return QColor::fromRgbF(
        first.redF() * x + second.redF() * (1 - x),
        first.greenF() * x + second.greenF() * (1 - x),
        first.blueF() * x + second.blueF() * (1 - x)
    );
}

QColor piecewise_linear_interpolator(const QVector<Node> & nodes, double position) {
    if (position < nodes.first().position) {
        return nodes.first().colour;
    }

    for (QVector<Node>::const_iterator node = nodes.cbegin(); node + 1 != nodes.cend(); ++node) {
        if ((node->position <= position) && (position <= (node + 1)->position)) {
            const Node & left = *node;
            const Node & right = *(node + 1);
            return linear_interpolate(left.colour, left.position, right.colour, right.position, position);
        }
    }

    return nodes.last().colour;
}

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

QColor Universe::altitude_colour(double altitude) {
    return piecewise_linear_interpolator({
        {-90.0, QColor::fromRgbF(0.0, 0.0, 0.0)},
        {-18.0, QColor::fromRgbF(0.0, 0.0, 0.25)},
        {  0.0, QColor::fromRgbF(0.0, 0.0, 1.0)},
        {  0.0, QColor::fromRgbF(1.0, 0.25, 0.0)},
        {  4.0, QColor::fromRgbF(1.0, 0.75, 0.0)},
        {  8.0, QColor::fromRgbF(0.0, 0.75, 1.0)},
        { 90.0, QColor::fromRgbF(0.0, 0.62, 0.87)}
    }, altitude);
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

QDateTime Universe::next_sun_crossing(double latitude, double longitude, double altitude, bool direction_up, int resolution) {
    QDateTime now = QDateTime::fromTime_t((QDateTime::currentDateTimeUtc().toTime_t() / resolution) * resolution);
    double oldalt = Universe::sun_altitude(latitude, longitude, now);

    for (int i = 1; i <= 86400 / resolution; ++i) {
        QDateTime instant = now.addSecs(resolution * i);
        double newalt = Universe::sun_altitude(latitude, longitude, instant);
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
