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
    QString m_id;
    double m_latitude;
    double m_longitude;
    double m_altitude;

    double m_darkness_limit;
    double m_humidity_limit;

    Storage *m_primary_storage;
    Storage *m_permanent_storage;

    QNetworkAccessManager *m_network_manager;
    QTimer *m_timer_automatic;
public:
    Station(const QString& id);
    ~Station(void);


    void set_storages(const QDir& primary_storage_dir, const QDir& permanent_storage_dir);
    Storage& primary_storage(void);
    Storage& permanent_storage(void);

    void set_position(const double new_latitude, const double new_longitude, const double new_altitude);
    double latitude(void) const;
    double longitude(void) const;
    double altitude(void) const;

    const QString& get_id(void) const;
    void set_id(const QString& new_id);

    bool is_dark(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    void set_darkness_limit(const double new_altitude_dark);
    double darkness_limit(void) const;

    bool is_humid(void) const;
    void set_humidity_limit(const double new_altitude_dark);
    double humidity_limit(void) const;

    bool manual = false;

    Polar sun_position(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double sun_altitude(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double sun_azimuth(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;

    Dome* dome;
    QVector<Server> servers;

    void check_sun(void);
    QJsonObject prepare_heartbeat(void) const;
public slots:
    void automatic_check(void);
};

#endif // STATION_H
