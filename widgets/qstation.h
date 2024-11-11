#ifndef QSTATION_H
#define QSTATION_H

#include <QGroupBox>
#include <QMessageBox>

#include "APC/APC_include.h"
#include "utils/state/stationstate.h"
#include "widgets/qdome.h"
#include "widgets/qserver.h"
#include "widgets/qcamera.h"

#include "logging/statelogger.h"

namespace Ui {
    class QStation;
}

class QStation: public QAmosWidget {
    Q_OBJECT
private:
    Ui::QStation * ui;

    // Position on the WGS84 spheroid
    const QDateTime m_start_time;
    double m_latitude;
    double m_longitude;
    double m_altitude;

    bool m_manual_control;
    bool m_safety_override;

    StationState m_state;
    StateLogger * m_state_logger;

    // Pointers to subordinate widgets
    QDome * m_dome;
    const QServer * m_server;
    const QCamera * m_camera_allsky;
    const QCamera * m_camera_spectral;

    QTimer * m_timer_automatic;
    QTimer * m_timer_heartbeat;

    void set_state(StationState new_state);

    void connect_slots(void) override;
    void load_defaults(void) override;
    void load_settings_inner(void) override;
    void save_settings_inner(void) const override;
    void apply_changes_inner(void) override;
    void discard_changes_inner(void) override;

    constexpr static double DefaultLatitude = 48.0;
    constexpr static double DefaultLongitude = 17.0;
    constexpr static double DefaultAltitude = 186.0;

private slots:
    void set_position(const double new_latitude, const double new_longitude, const double new_altitude);

    void automatic_timer(void);

    void on_cb_manual_clicked(bool checked);
    void on_cb_safety_override_clicked(bool checked);

public:
    const static StationState NotObserving, Observing, Daylight, Manual, DomeUnreachable, RainOrHumid, NoMasterPower, Inconsistent;
    constexpr static unsigned int HeartbeatInterval = 60000;

    explicit QStation(QWidget * parent = nullptr);
    ~QStation();

    bool is_changed(void) const override;

    static QString temperature_colour(float temperature);

    inline const QDateTime & start_time(void) const { return this->m_start_time; }

    // Manual control
    void set_manual_control(bool manual);
    inline bool is_manual(void) const { return this->m_manual_control; }

    // Safety override
    void set_safety_override(bool override);
    inline bool is_safety_overridden(void) const { return this->m_safety_override; }

    void set_cameras(const QCamera * const allsky, const QCamera * const spectral);
    inline const QCamera * camera_allsky(void) const { return this->m_camera_allsky; }
    inline const QCamera * camera_spectral(void) const { return this->m_camera_spectral; }

    // Dome getter and setter
    inline void set_dome(QDome * dome) { this->m_dome = dome; }
    inline QDome * dome(void) const { return this->m_dome; }

    // Server getter and setter
    void set_server(const QServer * const server);
    inline const QServer * server(void) const { return this->m_server; }

    // Position getters and setter
    inline double latitude(void) const { return this->m_latitude; }
    inline double longitude(void) const { return this->m_longitude; }
    inline double altitude(void) const { return this->m_altitude; }

    // Darkness limit getters and setters
    bool is_dark_allsky(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    bool is_dark_spectral(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;

    // Sun position functions
    Polar sun_position(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    Polar moon_position(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    double sun_altitude(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    double sun_azimuth(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    double moon_altitude(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    double moon_azimuth(const QDateTime & time = QDateTime::currentDateTimeUtc()) const;
    QDateTime next_sun_crossing(double altitude, bool direction_up, int resolution = 60) const;
    QDateTime next_moon_crossing(double altitude, bool direction_up, int resolution = 60) const;

    inline QString state_logger_filename(void) const { return this->m_state_logger->filename(); };
    inline StationState state(void) const { return this->m_state; };

    QJsonObject json(void) const;

public slots:
    void initialize(QSettings * settings) override;

    void automatic_cover(void);

    void log_state(void) const;
    void heartbeat(void) const;
    void send_heartbeat(void) const;

signals:
    void manual_mode_changed(bool manual);
    void safety_override_changed(bool overridden);

    void position_changed(void);
    void state_changed(StationState state);

    void automatic_action_allsky(bool is_dark, const QDateTime & open_since);
    void automatic_action_spectral(bool is_dark, const QDateTime & open_since);
};

#endif // QSTATION_H
