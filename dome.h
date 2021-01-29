#include <random>

#include <QSerialPort>
#include <QTimer>
#include <QJsonObject>

#ifndef DOMEMANAGER_H
#define DOMEMANAGER_H

#include "forward.h"

class Dome: public QObject {
    Q_OBJECT
private:
    constexpr static unsigned int REFRESH = 300;
    const static Request RequestBasic, RequestEnv, RequestShaft, RequestShaftOld;

    unsigned char m_address;
    QDateTime m_last_received;

    QSerialPort *m_serial_port;
    QTimer *m_refresh_timer;

    SerialBuffer *m_buffer;

    void update_status_basic(void);
    void update_status_environment(void);
    void update_status_shaft(void);

    void process_response_S(const QByteArray &response);
    void process_response_T(const QByteArray &response);
    void process_response_Z(const QByteArray &response);

    DomeStateS m_state_S;
    DomeStateT m_state_T;
    DomeStateZ m_state_Z;

    unsigned char m_robin = 0;
public:
    const static Command CommandNoOp;
    const static Command CommandOpenCover, CommandCloseCover;
    const static Command CommandFanOn, CommandFanOff;
    const static Command CommandIIOn, CommandIIOff;
    const static Command CommandHotwireOn, CommandHotwireOff;
    const static Command CommandResetSlave;

    const static SerialPortState SerialPortNotSet, SerialPortOpen, SerialPortError;

    Dome();
    ~Dome();

    void clear_serial_port(void);
    void set_serial_port(const QString &port);

    const QDateTime& last_received(void) const;
    SerialPortState serial_port_state(void) const;
    QString serial_port_info(void) const;

    const DomeStateS& state_S(void) const;
    const DomeStateT& state_T(void) const;
    const DomeStateZ& state_Z(void) const;

    void send_command(const Command &command) const;
    void send_request(const Request &request) const;
    void send(const QByteArray &message) const;

    QJsonObject json(void) const;

public slots:
    void request_status(void);
    void process_response(void);
    void process_message(const QByteArray &message);
    void handle_error(QSerialPort::SerialPortError error);

signals:
    void read_timeout(void) const;
    void write_timeout(void) const;
    void response_received(const QByteArray &response) const;

    void state_updated_S(void) const;
    void state_updated_T(void) const;
    void state_updated_Z(void) const;

    void cover_closed(int position) const;
    void cover_open(int position) const;
    void cover_moved(int position) const;

    void serial_port_changed(const QString &name) const;
};

#endif // DOMEMANAGER_H
