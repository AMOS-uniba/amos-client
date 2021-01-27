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
    void set_state(StationState new_state);

    FileSystemScanner *m_filesystemscanner;
    Storage *m_primary_storage;
    Storage *m_permanent_storage;

    StateLogger *m_state_logger;
    Dome *m_dome;
    Server *m_server;
    UfoManager *m_ufo_manager;

    QNetworkAccessManager *m_network_manager;
    QTimer *m_timer_automatic;
    QTimer *m_timer_file_watchdog;
public:
    const static StationState NotObserving, Observing, Daylight, Manual, DomeUnreachable, RainOrHumid, NoMasterPower;

    Station(const QString& id);
    ~Station(void);

    StationState state(void);

    void set_scanner(const QDir &directory);
    FileSystemScanner* scanner(void) const;


    Dome* dome(void) const;

    QJsonObject prepare_heartbeat(void) const;

    // G&S for storage
    void set_storages(const QDir& primary_storage_dir, const QDir& permanent_storage_dir);
    Storage* primary_storage(void) const;
    Storage* permanent_storage(void) const;

    void set_server(Server *server);
    Server* server(void);

    // G&S for position
    void set_position(const double new_latitude, const double new_longitude, const double new_altitude);
    double latitude(void) const;
    double longitude(void) const;
    double altitude(void) const;

    // G&S for ID
    const QString& get_id(void) const;
    void set_id(const QString& new_id);

    // Darkness limit getters and setters
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
    QDateTime next_sun_crossing(double altitude, bool direction_up, int resolution = 60) const;

    // UFo manager getters and setters
    void set_ufo_manager(UfoManager *ufo_manager);
    UfoManager* ufo_manager(void) const;


    // Manual control and
    void set_manual_control(bool manual);
    bool is_manual(void) const;

    void set_safety_override(bool override);
    bool is_safety_overridden(void) const;

    // Command wrappers
    void open_cover(void);
    void close_cover(void);

    void turn_on_intensifier(void);
    void turn_off_intensifier(void);

    void turn_on_hotwire(void);
    void turn_off_hotwire(void);

    void turn_on_fan(void);
    void turn_off_fan(void);

    QString state_logger_filename(void) const;

    static QString temperature_colour(float temperature);

public slots:
    void automatic_check(void);
    void file_check(void);
    void log_state(void);

    void send_heartbeat(void);
    void process_sightings(QVector<Sighting> sightings);

signals:
    void state_changed(StationState state) const;

    void id_changed(void) const;
    void position_changed(void) const;
    void darkness_limit_changed(double new_limit) const;
    void humidity_limits_changed(double new_low, double new_high) const;
};

#endif // STATION_H
