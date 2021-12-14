#ifndef QDOME_H
#define QDOME_H

#include <QGroupBox>
#include <QSerialPort>

#include "forward.h"
#include "utils/domestate.h"
#include "utils/qserialbuffer.h"

#include "widgets/qconfigurable.h"
#include "widgets/lines/qdisplayline.h"
#include "mainwindow.h"
#include "widgets/qdomewidget.h"
#include "widgets/qstation.h"

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
    constexpr static unsigned int Refresh = 300;                                // Robin time in ms

    Ui::QDome * ui;
    const QStation * m_station;

    QDateTime m_last_received;
    QDateTime m_open_since;

    QThread * m_thread;
    QSerialPortManager * m_spm;
    SerialPortState m_sps;
    QString m_data_state;

    QTimer * m_open_timer;

    // Humidity limits with hysteresis: open is humidity <= lower, close if humidity >= higher
    double m_humidity_limit_lower = 70.0;
    double m_humidity_limit_upper = 90.0;

    DomeStateS m_state_S;
    DomeStateT m_state_T;
    DomeStateZ m_state_Z;

    void send_command(const Command & command) const;

    void process_message(const QByteArray & message);

    void connect_slots(void) override;
    void load_defaults(void) override;
    void load_settings_inner(void) override;
    void save_settings_inner(void) const override;
    void apply_changes_inner(void) override;
    void discard_changes_inner(void) override;

    constexpr static double DefaultHumidityLower = 75.0;
    constexpr static double DefaultHumidityUpper = 90.0;
    const static QString DefaultPort;

private slots:
    void display_dome_state(void);
    void display_basic_data(const DomeStateS & state);
    void display_env_data(const DomeStateT & state);
    void display_shaft_data(const DomeStateZ & state);

    void display_data_state(void) const;

    void toggle_hotwire(void) const;
    void toggle_intensifier(void) const;
    void toggle_fan(void) const;

    void on_bt_cover_open_clicked();
    void on_bt_cover_close_clicked();

    void set_open_since(void);

public:
    const static Command CommandNoOp;
    const static Command CommandOpenCover, CommandCloseCover;
    const static Command CommandFanOn, CommandFanOff;
    const static Command CommandIIOn, CommandIIOff;
    const static Command CommandHotwireOn, CommandHotwireOff;
    const static Command CommandResetSlave;

    const static ValueFormatter<double> TemperatureValueFormatter, HumidityValueFormatter;
    const static ColourFormatter<double> TemperatureColourFormatter;

    explicit QDome(QWidget * parent = nullptr);
    ~QDome();

    virtual void initialize(QSettings * settings) override;
    bool is_changed(void) const override;

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
    inline bool is_humid(void) const { return (this->state_T().humidity_sht() >= this->humidity_limit_lower()); };
    inline bool is_very_humid(void) const { return (this->state_T().humidity_sht() >= this->humidity_limit_upper()); };
    inline double humidity_limit_lower(void) const { return this->m_humidity_limit_lower; };
    inline double humidity_limit_upper(void) const { return this->m_humidity_limit_upper; };

    void set_humidity_limits(const double new_humidity_lower, const double new_humidity_upper);

public slots:
    void set_formatters(void);

    void list_serial_ports(void);
    void set_serial_port_state(const SerialPortState state);
    void set_data_state(const QString & data_state);

    void handle_no_serial_port_set(void);
    void handle_serial_port_selected(const QString & port);
    void handle_serial_port_changed(const QString & port);
    void handle_serial_port_error(QSerialPort::SerialPortError error, const QString & message);

    void pass_log_message(Concern concern, Level level, const QString & message);

    // Command wrappers
    void open_cover(void) const;
    void close_cover(void) const;

    void turn_on_intensifier(void) const;
    void turn_off_intensifier(void) const;

    void turn_on_hotwire(void) const;
    void turn_off_hotwire(void) const;

    void turn_on_fan(void) const;
    void turn_off_fan(void) const;

signals:
    void command(const QByteArray & command) const;

    void state_updated(void);
    void state_updated_S(const DomeStateS & state);
    void state_updated_T(const DomeStateT & state);
    void state_updated_Z(const DomeStateZ & state);

    void cover_closed(int position) const;
    void cover_open(int position) const;
    void cover_moved(int position) const;

    void serial_port_selected(const QString & port);

    void humidity_limits_changed(double new_lower, double new_upper);
};

#endif // QDOME_H
