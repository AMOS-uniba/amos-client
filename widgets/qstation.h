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
    bool m_manual_control;
    bool m_safety_override;

    StationState m_state;
    StateLogger * m_state_logger;

    const QScannerBox * m_scanner;
    const QStorageBox * m_primary_storage;
    const QStorageBox * m_permanent_storage;

    const QDome * m_dome;
    const QServer * m_server;
    const QUfoManager * m_ufo_manager;
    const QUfoManager * m_ufo_hd_manager;

    QTimer * m_timer_automatic;
    QTimer * m_timer_heartbeat;

    void set_state(StationState new_state);

    bool is_changed(void) const;

private slots:
    void load_settings(void);
    void load_settings_inner(void);
    void load_defaults(void);
    void save_settings(void) const;

    void on_cb_manual_clicked(bool checked);
    void on_cb_safety_override_clicked(bool checked);

public:
    const static StationState NotObserving, Observing, Daylight, Manual, DomeUnreachable, RainOrHumid, NoMasterPower;
    constexpr static unsigned int HeartbeatInterval = 60000;

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

    // Scanner getter and setter
    void set_scanner(const QScannerBox * const scanner);
    const QScannerBox * scanner(void) const;

    // Storage getters and setters
    void set_storages(const QStorageBox * const primary_storage, const QStorageBox * const permanent_storage);
    const QStorageBox * primary_storage(void) const;
    const QStorageBox * permanent_storage(void) const;

    // Dome getter and setter
    void set_dome(const QDome * const dome);
    const QDome * dome(void) const;

    // Server getter and setter
    void set_server(const QServer * const server);
    const QServer * server(void) const;

    // UFO manager getter and setter
    void set_ufo_manager(const QUfoManager * const ufo_manager);
    const QUfoManager * ufo_manager(void) const;

    void set_ufo_hd_manager(const QUfoManager * const ufo_hd_manager);
    const QUfoManager * ufo_hd_manager(void) const;

    // Position getters and setter
    double latitude(void) const;
    double longitude(void) const;
    double altitude(void) const;

    // Darkness limit getters and setters
    bool is_dark(const QDateTime& time = QDateTime::currentDateTimeUtc()) const;
    double darkness_limit(void) const;

    // Sun position functions
    Polar sun_position(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    Polar moon_position(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    double sun_altitude(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    double sun_azimuth(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    double moon_altitude(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    double moon_azimuth(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    QDateTime next_sun_crossing(double altitude, bool direction_up, int resolution = 60) const;

    QString state_logger_filename(void) const;

    StationState state(void) const;
    QJsonObject prepare_heartbeat(void) const;

public slots:
    void initialize(void);

    void set_position(const double new_latitude, const double new_longitude, const double new_altitude);
    void set_darkness_limit(const double new_altitude_dark);

    void apply_settings(void);
    void apply_settings_inner(void);
    void discard_settings(void);
    void handle_settings_changed(void);

    void automatic_check(void);
    void automatic_ufo(void);
    void log_state(void) const;

    void heartbeat(void) const;
    void send_heartbeat(void) const;
    void process_sightings(QVector<Sighting> sightings);

signals:
    void settings_changed(void) const;

    void manual_mode_changed(bool manual);
    void safety_override_changed(bool overridden);

    void id_changed(void) const;
    void position_changed(void) const;
    void darkness_limit_changed(double new_limit) const;
    void humidity_limits_changed(double new_low, double new_high) const;

    void state_changed(StationState state) const;
};

#endif // QSTATION_H
