#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVector>
#include <QNetworkAccessManager>
#include <QStorageInfo>

#include "forward.h"
#include "APC/APC_include.h"

#ifndef STATION_H
#define STATION_H

struct Position {
    double latitude;
    double longitude;
    double altitude;
};

/*!
 * \brief The Station class represents the entire AMOS station (computer, dome, camera)
 */
class Station: public QObject {
    Q_OBJECT
private:
    QString id;
    double altitude_dark;

    Storage *primary_storage;
    Storage *permanent_storage;

    QNetworkAccessManager *network_manager;
public:
    Station(const QString& _id, const QDir& primary_storage_dir, const QDir& permanent_storage_dir);
    ~Station(void);

    double latitude;
    double longitude;
    double altitude;

    void set_position(const double new_latitude, const double new_longitude, const double new_altitude);
    const QString& get_id(void) const;
    void set_id(const QString& new_id);

    bool is_dark(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    void set_altitude_dark(const double new_altitude_dark);

    bool automatic = false;

    Polar sun_position(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double get_sun_altitude(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double get_sun_azimuth(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;

    Dome* dome_manager;
    QVector<Server> servers;

    void check_sun(void);
    QJsonObject prepare_heartbeat(void) const;

    Storage& get_primary_storage(void);
    Storage& get_permanent_storage(void);

    QStorageInfo storage_info_permanent() const;
};

#endif // STATION_H
