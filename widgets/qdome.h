#ifndef QDOME_H
#define QDOME_H

#include <QGroupBox>
#include <QSerialPort>

#include "utils/domestate.h"

#include "utils/qserialportmanager.h"
#include "widgets/qconfigurable.h"
#include "widgets/lines/qdisplayline.h"

QT_FORWARD_DECLARE_CLASS(QStation);

namespace Ui {
    class QDome;
}

/**
 * @brief The QDome class handles the communication and control of the AMOS dome
 * Provides its own widget with settings, configuration and display
 */
class QDome: public QAmosWidget {
    Q_OBJECT
private:
    constexpr static unsigned int Refresh = 300;    // Robin time in ms

    Ui::QDome * ui;
    const QStation * m_station;

    QDateTime m_last_received;
    QDateTime m_open_since;
    /* When the weather last became clear, or invalid if it is not clear now. Rain and humidity are
     * the only conditions that can flap, so this latch covers exactly those two and not darkness:
     * counting the sun would reset it every dawn and delay every dusk opening by the whole settle
     * time, for nothing.
     */
    QDateTime m_weather_clear_since;
    bool m_enabled;

    QThread * m_thread;
    QSerialPortManager * m_spm;
    SerialPortState m_sps;
    QString m_data_state;

    QTimer * m_open_timer;

    // Humidity limits with hysteresis: open is humidity <= lower, close if humidity >= higher
    double m_humidity_limit_lower = 70.0;
    double m_humidity_limit_upper = 90.0;

    /* Whether a usable humidity reading is a precondition for observing, and whether one has ever
     * arrived in this session. A station whose sensor has never produced anything keeps working
     * rather than going dark unattended; one whose sensor worked and then stopped is guarded.
     */
    bool m_require_humidity = true;
    bool m_humidity_seen = false;
    mutable bool m_humidity_guard_engaged = false;

    /* How long the weather must have been clear before the cover may open, in seconds. Zero waits
     * for nothing, which is the behaviour of every release before this one.
     */
    int m_open_settle_time = 600;
    mutable bool m_settle_wait_logged = false;

    DomeStateS m_state_S;
    DomeStateT m_state_T;
    DomeStateZ m_state_Z;

    void process_message(const QByteArray & message);

    // Whether the humidity reading may be acted upon at all: seen at least once, currently fresh,
    // and not contradicted by the dome's own SHT31 error bit.
    bool humidity_trustworthy(void) const;

    void connect_slots(void) override;
    void load_defaults(void) override;
    void load_settings_inner(void) override;
    void save_settings_inner(void) const override;
    void apply_changes_inner(void) override;
    void discard_changes_inner(void) override;

    constexpr static double DefaultHumidityLower = 75.0;
    constexpr static double DefaultHumidityUpper = 90.0;
    constexpr static bool DefaultRequireHumidity = true;

    /* How long the humidity may stay untrustworthy before the cover is closed, in seconds. The T
     * state arrives about every 750 ms against a two-second validity window, so a couple of dropped
     * telegrams must not slam the cover shut and cost the rest of the night.
     */
    constexpr static double HumidityGrace = 60.0;

    /* Default settle time before opening, in seconds, and the largest one that may be configured.
     * Ten minutes is about what a passing drizzle takes to clear, and a rain sensor flapping faster
     * than that never satisfies it -- which is the point. Raise it on a station whose sensor is
     * known to be unreliable.
     */
    constexpr static int DefaultOpenSettle = 600;
    constexpr static int MaxOpenSettle = 3600;

    constexpr static bool DefaultEnabled = true;
    const static QString DefaultPort;

private slots:
    void send_command(const Command & command);

    void display_dome_state(void);
    void display_basic_data(const DomeStateS & state);
    void display_env_data(const DomeStateT & state);
    void display_shaft_data(const DomeStateZ & state);

    void display_data_state(void) const;

    void toggle_hotwire(void);
    void toggle_intensifier(void);
    void toggle_fan(void);

    void on_bt_cover_open_clicked();
    void on_bt_cover_close_clicked();

    void set_open_since(void);
    void set_weather_clear_since(void);
    void set_enabled(int enable);

    void on_dsb_humidity_limit_upper_valueChanged(double value);
    void on_dsb_humidity_limit_lower_valueChanged(double value);
    void on_cb_require_humidity_toggled(bool checked);
    void on_sb_open_settle_valueChanged(int value);

public:
    const static Command CommandNoOp;
    const static Command CommandOpenCover, CommandCloseCover;
    const static Command CommandFanOn, CommandFanOff;
    const static Command CommandIIOn, CommandIIOff;
    const static Command CommandHotwireOn, CommandHotwireOff;
    const static Command CommandSoftwareReset;

    const static ValueFormatter<double> TemperatureValueFormatter, HumidityValueFormatter;

    explicit QDome(QWidget * parent = nullptr);
    ~QDome();

    virtual void initialize(QSettings * settings) override;
    bool is_changed(void) const override;
    inline bool is_enabled(void) const { return this->m_enabled; }

    inline const QDateTime & last_received(void) const { return this->m_last_received; };
    inline const QDateTime & open_since(void) const { return this->m_open_since; };
    inline SerialPortState serial_port_state(void) const { return this->m_sps; };
    inline QString data_state(void) const { return this->m_data_state; };

    QJsonObject json(void) const;

    inline const DomeStateS & state_S(void) const { return this->m_state_S; };
    inline const DomeStateT & state_T(void) const { return this->m_state_T; };
    inline const DomeStateZ & state_Z(void) const { return this->m_state_Z; };

    QString status_line(void) const;

    void set_station(const QStation * const station);

    // Humidity getters and setters
    /* Both fail towards safety when the reading cannot be trusted, but not symmetrically: refusing to
     * open costs nothing but a wait, whereas forcing a close costs the night, so only the latter
     * waits out HumidityGrace first.
     */
    bool is_humid(void) const;
    bool is_very_humid(void) const;
    inline bool requires_humidity(void) const { return this->m_require_humidity; };
    inline double humidity_limit_lower(void) const { return this->m_humidity_limit_lower; };
    inline double humidity_limit_upper(void) const { return this->m_humidity_limit_upper; };

    void set_humidity_limits(const double new_humidity_lower, const double new_humidity_upper);

    /* Whether the weather has been clear long enough for the cover to open.
     *
     * A faulty rain sensor that flips between raining and not raining used to cycle the cover: the
     * firmware closes on rain, and the moment the bit cleared the client opened again. Rain is only
     * ever a gate on opening here -- the client has no close-on-rain of its own -- so requiring the
     * all-clear to hold for a while is the whole of what this side can do about it, and it is
     * enough to break the loop.
     */
    bool weather_settled(void) const;
    inline int open_settle_time(void) const { return this->m_open_settle_time; };

    void set_open_settle_time(const int new_settle_time);

public slots:
    void set_formatters(void);

    void list_serial_ports(void);
    void set_serial_port_state(const SerialPortState & state);
    void set_data_state(const QString & data_state);

    void handle_serial_port_selected(const QString & port);
    void handle_serial_port_changed(const QString & port);
    void handle_serial_port_error(const QString & port, QSerialPort::SerialPortError error, const QString & message);

    // Command wrappers
    void open_cover(void);
    void close_cover(void);
    void request_sw_reset(void);

    void turn_on_intensifier(void);
    void turn_off_intensifier(void);
    void turn_on_hotwire(void);
    void turn_off_hotwire(void);
    void turn_on_fan(void);
    void turn_off_fan(void);

    void pass_log_message(Concern concern, Level level, const QString & message);

signals:
    void command(const QByteArray & command);

    void state_updated(void);
    void state_updated_S(const DomeStateS & state);
    void state_updated_T(const DomeStateT & state);
    void state_updated_Z(const DomeStateZ & state);

    void cover_closed(int position);
    void cover_open(int position);
    void cover_moved(int position);

    void enabled_set(int enabled);
    void serial_port_selected(const QString & port);
    void humidity_limits_changed(double new_lower, double new_upper);
};

#endif // QDOME_H
