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
    double m_humidity_limit_lower, m_humidity_limit_upper;
    bool m_manual_control;
    bool m_safety_override;

    StationState m_state;

    Storage *m_primary_storage;
    Storage *m_permanent_storage;

    StateLogger *m_state_logger;
    Dome *m_dome;
    Server *m_server;

    QNetworkAccessManager *m_network_manager;
    QTimer *m_timer_automatic, *m_timer_file_watchdog;

    QString m_ufo_path;
public:
    Station(const QString& id);
    ~Station(void);

    StationState determine_state(void) const;
    void check_state(void);
    StationState state(void);

    // G&S for storage
    void set_storages(const QDir& primary_storage_dir, const QDir& permanent_storage_dir);
    Storage& primary_storage(void);
    Storage& permanent_storage(void);

    void set_server(Server *server);
    Server* server(void);

    // G&S for position
    void set_position(const double new_latitude, const double new_longitude, const double new_altitude);
    double latitude(void) const;
    double longitude(void) const;
    double altitude(void) const;

    const QString& get_id(void) const;
    void set_id(const QString& new_id);

    // Darkness getters and setters
    bool is_dark(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    void set_darkness_limit(const double new_altitude_dark);
    double darkness_limit(void) const;

    // Humidity getters and setters
    bool is_humid(void) const;
    bool is_very_humid(void) const;
    void set_humidity_limits(const double new_humidity_lower, const double new_humidity_upper);
    double humidity_limit_lower(void) const;
    double humidity_limit_upper(void) const;

    // Sun position functions
    Polar sun_position(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double sun_altitude(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double sun_azimuth(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;

    void set_manual_control(bool manual);
    bool is_manual(void) const;

    void set_safety_override(bool override);
    bool is_safety_overridden(void) const;

    Dome* dome(void) const;

    QJsonObject prepare_heartbeat(void) const;

    void send_sighting(const Sighting &sighting);
    void move_sighting(Sighting &sighting);

    // Command wrappers
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
    void file_check(void);
    void log_state(void);

    void send_heartbeat(void);

signals:
    void state_changed(void) const;
};

#endif // STATION_H
