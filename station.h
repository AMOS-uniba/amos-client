#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVector>
#include <QNetworkAccessManager>

#include "domemanager.h"
#include "server.h"

#ifndef STATION_H
#define STATION_H


class Station {
private:
    double latitude;
    double longitude;
    double altitude;

    QNetworkAccessManager *network_manager;
public:
    Station();

    bool automatic = false;

    double get_sun_altitude(void) const;
    double get_sun_azimuth(void) const;

    DomeManager dome_manager;
    QVector<Server> servers;

    QJsonObject prepare_heartbeat(void) const;

};

#endif // STATION_H
