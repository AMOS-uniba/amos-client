#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>

#include "domemanager.h"

#ifndef STATION_H
#define STATION_H


class Station {
private:
    bool automatic = false;

    double latitude;
    double longitude;
    double altitude;

public:
    Station();

    double get_sun_altitude(void) const;
    double get_sun_azimuth(void) const;

    DomeManager dome_manager;

    QJsonObject prepare_heartbeat(void) const;
};

#endif // STATION_H
