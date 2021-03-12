#ifndef QDOME_H
#define QDOME_H

#include <QGroupBox>

#include "forward.h"
#include "utils/domestate.h"
#include "mainwindow.h"
#include "widgets/qdomewidget.h"

namespace Ui {
    class QDome;
}

class QDome : public QGroupBox {
    Q_OBJECT
private:
    constexpr static unsigned int REFRESH = 300;
    const static Request RequestBasic, RequestEnv, RequestShaft, RequestShaftOld;

    Ui::QDome *ui;
    Station *m_station;

    const unsigned char m_address = 0x99;
    QDateTime m_last_received;

    QSerialPort *m_serial_port;
    unsigned char m_robin = 0;
    QTimer *m_robin_timer;
    QTimer *m_serial_watchdog;

    SerialBuffer *m_buffer;

    double m_humidity_limit_lower = 70;
    double m_humidity_limit_upper = 90;

    DomeStateS m_state_S;
    DomeStateT m_state_T;
    DomeStateZ m_state_Z;

    void process_message(const QByteArray &message);
public:
    const static Command CommandNoOp;
    const static Command CommandOpenCover, CommandCloseCover;
    const static Command CommandFanOn, CommandFanOff;
    const static Command CommandIIOn, CommandIIOff;
    const static Command CommandHotwireOn, CommandHotwireOff;
    const static Command CommandResetSlave;

    const static SerialPortState SerialPortNotSet, SerialPortOpen, SerialPortError;

    explicit QDome(QWidget *parent = nullptr);
    ~QDome();

    static QColor temperature_colour(double temperature);

    void send_command(const Command &command) const;
    void send_request(const Request &request) const;
    void send(const QByteArray &message) const;

    const QDateTime& last_received(void) const;
    SerialPortState serial_port_state(void) const;

    QJsonObject json(void) const;

    const DomeStateS& state_S(void) const;
    const DomeStateT& state_T(void) const;
    const DomeStateZ& state_Z(void) const;

    QString status_line(void) const;

    void initialize(void);
    void load_settings(void);

    // Humidity getters and setters
    bool is_humid(void) const;
    bool is_very_humid(void) const;
    double humidity_limit_lower(void) const;
    double humidity_limit_upper(void) const;
    void set_humidity_limits(const double new_humidity_lower, const double new_humidity_upper);

private slots:
    void display_dome_state(void);

    void display_basic_data(const DomeStateS &state);
    void display_env_data(const DomeStateT &state);
    void display_shaft_data(const DomeStateZ &state);

    void toggle_hotwire(void);
    void toggle_intensifier(void);
    void toggle_fan(void);

    void handle_humidity_limits_changed(void);
    void humidity_limits_apply(void);
    void humidity_limits_discard(void);

public slots:
    void set_formatters(double humidity_limit_lower, double humidity_limit_upper);

    void list_serial_ports(void);
    void clear_serial_port(void);
    void set_serial_port(const QString &port);
    void check_serial_port(void);

    void display_serial_port_info(void) const;

    void process_response(void);
    void handle_error(QSerialPort::SerialPortError error);

    void request_status(void);

    // Command wrappers
    void open_cover(void);
    void close_cover(void);

    void turn_on_intensifier(void);
    void turn_off_intensifier(void);

    void turn_on_hotwire(void);
    void turn_off_hotwire(void);

    void turn_on_fan(void);
    void turn_off_fan(void);

signals:
    void state_updated_S(const DomeStateS &state);
    void state_updated_T(const DomeStateT &state);
    void state_updated_Z(const DomeStateZ &state);

    void cover_closed(int position) const;
    void cover_open(int position) const;
    void cover_moved(int position) const;

    void serial_port_changed(const QString &port);

    void humidity_limits_changed(double new_lower, double new_upper);
};

#endif // QDOME_H
