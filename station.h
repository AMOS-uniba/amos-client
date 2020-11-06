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
    bool m_manual_control;
    bool m_safety_override;

    Storage *m_primary_storage;
    Storage *m_permanent_storage;

    QNetworkAccessManager *m_network_manager;
    QTimer *m_timer_automatic;
public:
    Station(const QString& id);
    ~Station(void);

    // G&S for storage
    void set_storages(const QDir& primary_storage_dir, const QDir& permanent_storage_dir);
    Storage& primary_storage(void);
    Storage& permanent_storage(void);

    // G&S for position
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

    Polar sun_position(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double sun_altitude(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double sun_azimuth(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;

    void set_manual_control(bool manual);
    bool is_manual(void) const;

    void set_safety_override(bool override);
    bool is_safety_overridden(void) const;

    Dome* dome;
    QVector<Server> servers;

    QJsonObject prepare_heartbeat(void) const;

    void open_cover(void);
    void close_cover(void);

    void turn_on_intensifier(void);
    void turn_off_intensifier(void);

    void turn_on_hotwire(void);
    void turn_off_hotwire(void);

    void turn_on_fan(void);
    void turn_off_fan(void);

public slots:
    void automatic_check(void);
};

#endif // STATION_H
