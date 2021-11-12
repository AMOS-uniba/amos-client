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
    unsigned char m_robin = 0;

    const static Request RequestBasic, RequestEnv, RequestShaft;

    Ui::QDome * ui;
    const QStation * m_station;

    constexpr static unsigned char Address = 0x99;                              // address
    QDateTime m_last_received;
    QDateTime m_open_since;

    QDomeThread * m_dome_thread;

    QTimer * m_robin_timer;
    QTimer * m_serial_watchdog;
    QTimer * m_open_timer;

    // Humidity limits with hysteresis: open is humidity <= lower, close if humidity >= higher
    double m_humidity_limit_lower = 70.0;
    double m_humidity_limit_upper = 90.0;

    DomeStateS m_state_S;
    DomeStateT m_state_T;
    DomeStateZ m_state_Z;

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

    const static SerialPortState SerialPortNotSet, SerialPortOpen, SerialPortError;

    const static ValueFormatter<double> TemperatureValueFormatter, HumidityValueFormatter;
    const static ColourFormatter<double> TemperatureColourFormatter;

    explicit QDome(QWidget * parent = nullptr);
    ~QDome();

    virtual void initialize(QSettings * settings) override;
    bool is_changed(void) const override;

    void send_command(const Command & command) const;
    void send_request(const Request & request) const;
    void send(const QByteArray & message) const;

    inline const QDateTime & last_received(void) const { return this->m_last_received; };
    inline const QDateTime & open_since(void) const { return this->m_open_since; };

    SerialPortState serial_port_state(void) const;

    QJsonObject json(void) const;

    inline const DomeStateS & state_S(void) const { return this->m_state_S; };
    inline const DomeStateT & state_T(void) const { return this->m_state_T; };
    inline const DomeStateZ & state_Z(void) const { return this->m_state_Z; };

    QString status_line(void) const;

    void set_station(const QStation * const station);

    // Humidity getters and setters
    bool is_humid(void) const;
    bool is_very_humid(void) const;
    double humidity_limit_lower(void) const;
    double humidity_limit_upper(void) const;
    void set_humidity_limits(const double new_humidity_lower, const double new_humidity_upper);

public slots:
    void set_formatters(void);

    void list_serial_ports(void);
 //   void clear_serial_port(void);
    void set_serial_port(const QString & port);
    void check_serial_port(void);

    void display_serial_port_info(void) const;

    void handle_error(QSerialPort::SerialPortError error, const QString & message);

    void request_status(void);

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
    void request_state(const QByteArray & request) const;

    void state_updated_S(const DomeStateS & state);
    void state_updated_T(const DomeStateT & state);
    void state_updated_Z(const DomeStateZ & state);

    void cover_closed(int position) const;
    void cover_open(int position) const;
    void cover_moved(int position) const;

    void serial_port_changed(const QString & port);

    void humidity_limits_changed(double new_lower, double new_upper);
};

#endif // QDOME_H
