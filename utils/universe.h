#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <QDateTime>

#include "forward.h"
#include "APC\APC_include.h"

namespace Universe {
    constexpr static double delta_t = 67.28 / 86400.0;

    Vec3D compute_sun_ecl(const QDateTime &time = QDateTime::currentDateTimeUtc());
    Vec3D compute_sun_equ(const QDateTime &time = QDateTime::currentDateTimeUtc());
    Vec3D compute_moon_equ(const QDateTime &time = QDateTime::currentDateTimeUtc());
    QColor altitude_colour(double altitude);

    double mjd(const QDateTime &time = QDateTime::currentDateTimeUtc());
    double julian_centuries(const QDateTime &time = QDateTime::currentDateTimeUtc());
};

#endif // UNIVERSE_H
