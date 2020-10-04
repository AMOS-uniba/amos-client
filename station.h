#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVector>
#include <QNetworkAccessManager>
#include <QStorageInfo>

#include "domemanager.h"
#include "server.h"
#include "APC/APC_include.h"

#ifndef STATION_H
#define STATION_H

struct Position {
    double latitude;
    double longitude;
    double altitude;
};

class Station: public QObject {
    Q_OBJECT
private:
    Storage *primary_storage;
    Storage *permanent_storage;

    QNetworkAccessManager *network_manager;
public:
    double latitude;
    double longitude;
    double altitude;
    QString id;

    Station(const QString& _id, const QDir& primary_storage_dir, const QDir& permanent_storage_dir);

    bool automatic = false;

    Polar sun_position(const QDateTime& time = QDateTime::currentDateTimeUtc());
    double get_sun_altitude(void);
    double get_sun_azimuth(void);

    DomeManager dome_manager;
    QVector<Server> servers;

    QJsonObject prepare_heartbeat(void) const;

    Storage& get_primary_storage(void);
    Storage& get_permanent_storage(void);

    QStorageInfo storage_info_permanent() const;
};

#endif // STATION_H
