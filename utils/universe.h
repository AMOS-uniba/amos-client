#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <QDateTime>

#include "APC\APC_include.h"

namespace Universe {
    constexpr static double delta_t = 67.28 / 86400.0;

    Vec3D compute_sun_ecl(const QDateTime & time = QDateTime::currentDateTimeUtc());
    Vec3D compute_sun_equ(const QDateTime & time = QDateTime::currentDateTimeUtc());
    Vec3D compute_moon_equ(const QDateTime & time = QDateTime::currentDateTimeUtc());

    double mjd(const QDateTime & time = QDateTime::currentDateTimeUtc());
    double julian_centuries(const QDateTime & time = QDateTime::currentDateTimeUtc());

    Polar sun_position(const double latitude, const double longitude, const QDateTime & time = QDateTime::currentDateTimeUtc());
    Polar moon_position(const double latitude, const double longitude, const QDateTime & time = QDateTime::currentDateTimeUtc());

    double sun_altitude(const double latitude, const double longitude, const QDateTime & time = QDateTime::currentDateTimeUtc());
    double sun_azimuth(const double latitude, const double longitude, const QDateTime & time = QDateTime::currentDateTimeUtc());
    double moon_altitude(const double latitude, const double longitude, const QDateTime & time = QDateTime::currentDateTimeUtc());
    double moon_azimuth(const double latitude, const double longitude, const QDateTime & time = QDateTime::currentDateTimeUtc());

    QDateTime next_crossing(std::function<double(double, double, QDateTime)> fun,
                            double latitude, double longitude, double altitude,
                            bool direction_up, int resolution = 60);
};

#endif // UNIVERSE_H
