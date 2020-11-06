#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <QDateTime>

#include "forward.h"
#include "APC\APC_include.h"

class Universe {
public:
    constexpr static double delta_t = 67.28 / 86400.0;

    Universe();

    static Vec3D compute_sun_ecl(const QDateTime& time = QDateTime::currentDateTimeUtc());
    static Vec3D compute_sun_equ(const QDateTime& time = QDateTime::currentDateTimeUtc());
    static QString altitude_colour(double altitude);

    static double mjd(const QDateTime& time = QDateTime::currentDateTimeUtc());
    static double julian_centuries(const QDateTime& time = QDateTime::currentDateTimeUtc());
};

#endif // UNIVERSE_H
