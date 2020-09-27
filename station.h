#include <QDateTime>

#ifndef STATION_H
#define STATION_H


class Station {
private:
    double latitude;
    double longitude;
    double altitude;
public:
    Station();

    double get_sun_altitude(void) const;
    double get_sun_azimuth(void) const;

    QJsonDocument prepare_heartbeat(void) const;
};

#endif // STATION_H
