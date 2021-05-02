#ifndef QSTATION_H
#define QSTATION_H

#include <QGroupBox>

#include "forward.h"
#include "APC/APC_include.h"

#include "logging/statelogger.h"

namespace Ui {
    class QStation;
}

class QStation: public QGroupBox {
    Q_OBJECT
private:
    Ui::QStation *ui;

    double m_latitude;
    double m_longitude;
    double m_altitude;

    double m_darkness_limit;
    double m_humidity_limit_lower;
    double m_humidity_limit_upper;
    bool m_manual_control;
    bool m_safety_override;

    StationState m_state;

    QScannerBox *m_scanner;
    QStorageBox *m_primary_storage;
    QStorageBox *m_permanent_storage;

    StateLogger *m_state_logger;
    QDome *m_dome;
    QServer *m_server;
    UfoManager *m_ufo_manager;

    QTimer *m_timer_automatic;
    QTimer *m_timer_file_watchdog;

    void set_state(StationState new_state);

private slots:
    void on_cb_manual_stateChanged(int enable);

public:
    const static StationState NotObserving, Observing, Daylight, Manual, DomeUnreachable, RainOrHumid, NoMasterPower;

    explicit QStation(QWidget *parent = nullptr);
    ~QStation();

    static QString temperature_colour(float temperature);

    // Manual control
    void set_manual_control(bool manual);
    bool is_manual(void) const;

    // Safety override
    void set_safety_override(bool override);
    bool is_safety_overridden(void) const;

    // Station name
    void set_name(const QString & name);
    const QString& name(void) const;

    // Dome getter and setter
    void set_dome(QDome * const dome);
    QDome* dome(void) const;

    // Scanner getter and setter
    void set_scanner(QScannerBox * const scanner);
    QScannerBox* scanner(void) const;

    // Storage getters and setters
    void set_storages(QStorageBox * const primary_storage, QStorageBox * const permanent_storage);
    QStorageBox* primary_storage(void) const;
    QStorageBox* permanent_storage(void) const;

    // Server getter and setter
    void set_server(QServer * const server);
    QServer* server(void) const;

    // UFO manager getter and setter
    void set_ufo_manager(UfoManager * const ufo_manager);
    UfoManager* ufo_manager(void) const;

    // Position getters and setter
    void set_position(const double new_latitude, const double new_longitude, const double new_altitude);
    double latitude(void) const;
    double longitude(void) const;
    double altitude(void) const;

    // Darkness limit getters and setters
    bool is_dark(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    void set_darkness_limit(const double new_altitude_dark);
    double darkness_limit(void) const;

    // Sun position functions
    Polar sun_position(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    Polar moon_position(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double sun_altitude(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double sun_azimuth(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double moon_altitude(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double moon_azimuth(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    QDateTime next_sun_crossing(double altitude, bool direction_up, int resolution = 60) const;

    QString state_logger_filename(void) const;

    StationState state(void) const;
    QJsonObject prepare_heartbeat(void) const;

public slots:
    void automatic_check(void);
    void file_check(void);
    void log_state(void);

    void send_heartbeat(void);
    void process_sightings(QVector<Sighting> sightings);

signals:
    void manual_mode_changed(bool manual);
    void id_changed(void) const;
    void position_changed(void) const;
    void darkness_limit_changed(double new_limit) const;
    void humidity_limits_changed(double new_low, double new_high) const;

    void state_changed(StationState state) const;
};

#endif // QSTATION_H
