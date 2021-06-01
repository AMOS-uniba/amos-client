#ifndef QDOME_H
#define QDOME_H

#include <QGroupBox>
#include <QSerialPort>

#include "forward.h"
#include "utils/domestate.h"
#include "utils/serialbuffer.h"

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
class QDome: public QGroupBox {
    Q_OBJECT
private:
    constexpr static unsigned int Refresh = 200;                                // Robin time in ms
    unsigned char m_robin = 0;

    const static Request RequestBasic, RequestEnv, RequestShaft, RequestShaftOld;

    Ui::QDome * ui;
    const QStation * m_station;

    constexpr static unsigned char Address = 0x99;                              // address
    QDateTime m_last_received;

    QSerialPort * m_serial_port;
    QTimer * m_robin_timer;
    QTimer * m_serial_watchdog;

    SerialBuffer * m_buffer;

    // Humidity limits with hysteresis: open is humidity <= lower, close if humidity >= higher
    double m_humidity_limit_lower = 70.0;
    double m_humidity_limit_upper = 90.0;

    DomeStateS m_state_S;
    DomeStateT m_state_T;
    DomeStateZ m_state_Z;

    void process_message(const QByteArray & message);
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

    void send_command(const Command & command) const;
    void send_request(const Request & request) const;
    void send(const QByteArray & message) const;

    const QDateTime & last_received(void) const;
    SerialPortState serial_port_state(void) const;

    QJsonObject json(void) const;

    const DomeStateS & state_S(void) const;
    const DomeStateT & state_T(void) const;
    const DomeStateZ & state_Z(void) const;

    QString status_line(void) const;

    void initialize(const QStation * const station);

    // Humidity getters and setters
    bool is_humid(void) const;
    bool is_very_humid(void) const;
    double humidity_limit_lower(void) const;
    double humidity_limit_upper(void) const;
    void set_humidity_limits(const double new_humidity_lower, const double new_humidity_upper);

private slots:
    void load_settings(void);

    void display_dome_state(void);

    void display_basic_data(const DomeStateS & state);
    void display_env_data(const DomeStateT & state);
    void display_shaft_data(const DomeStateZ & state);

    void toggle_hotwire(void) const;
    void toggle_intensifier(void) const;
    void toggle_fan(void) const;

    void handle_settings_changed(void);
    void apply_settings(void);
    void discard_settings(void);

public slots:
    void set_formatters(void);

    void list_serial_ports(void);
    void clear_serial_port(void);
    void set_serial_port(const QString & port);
    void check_serial_port(void);

    void display_serial_port_info(void) const;

    void process_response(void);
    void handle_error(QSerialPort::SerialPortError error);

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
    void state_updated_S(const DomeStateS & state);
    void state_updated_T(const DomeStateT & state);
    void state_updated_Z(const DomeStateZ & state);

    void cover_closed(int position) const;
    void cover_open(int position) const;
    void cover_moved(int position) const;

    void serial_port_changed(const QString & port);

    void settings_changed(double new_lower, double new_upper);
};

#endif // QDOME_H
