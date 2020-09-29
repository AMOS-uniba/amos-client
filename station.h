#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVector>
#include <QNetworkAccessManager>

#include "domemanager.h"
#include "server.h"
#include "APC/APC_include.h"

#ifndef STATION_H
#define STATION_H


class Station {
private:
    double sun_azimuth, sun_altitude;

    QNetworkAccessManager *network_manager;
public:
    double latitude;
    double longitude;
    double altitude;

    Station();

    bool automatic = false;

    Polar sun_position(const QDateTime& time = QDateTime::currentDateTimeUtc());
    double get_sun_altitude(void);
    double get_sun_azimuth(void);

    DomeManager dome_manager;
    QVector<Server> servers;

    QJsonObject prepare_heartbeat(void) const;

};

#endif // STATION_H
