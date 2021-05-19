#include "include.h"

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

QColor linear_interpolator(const QVector<Node> nodes, double position) {
    if (position < nodes.first().position) {
        return nodes.first().colour;
    }

    if (position > nodes.last().position) {
        return nodes.last().colour;
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
    return linear_interpolator({
        {-90.0, QColor::fromRgbF(0.0, 0.0, 0.0)},
        {-18.0, QColor::fromRgbF(0.0, 0.0, 0.25)},
        {  0.0, QColor::fromRgbF(0.0, 0.0, 1.0)},
        {  0.0, QColor::fromRgbF(1.0, 0.25, 0.0)},
        {  4.0, QColor::fromRgbF(1.0, 0.75, 0.0)},
        {  8.0, QColor::fromRgbF(0.0, 0.75, 1.0)},
        { 90.0, QColor::fromRgbF(0.0, 0.62, 0.87)}
    }, altitude);
}
