#include <random>

#include <QSerialPort>
#include <QTimer>
#include <QJsonObject>

#ifndef DOMEMANAGER_H
#define DOMEMANAGER_H

#include "forward.h"
#include "domestate.h"
#include "serialbuffer.h"

enum class CoverState {
    OPEN,
    OPENING,
    CLOSING,
    CLOSED,
    SAFETY,
    UNKNOWN,
};                              // we might split OPENING and CLOSING to include
                                // the information if it is before or after SAFETY

enum class TernaryState {
    ON,
    OFF,
    UNKNOWN,
};

struct State {
    char code;
    QString display_name;
};

struct CommandInfo {
    char code;
    QString display_name;
};

/*
class CommThread: public QThread {
    Q_OBJECT
public:
    explicit CommThread(QObject *parent = nullptr);
    ~CommThread(void);

    void transaction(const QString &port_name, int wait_timeout, const QByteArray &request);
signals:
    void response(const QByteArray &message);
    void error(const QString &message);
    void timeout(const QString &message);
private:
    void run(void) override;

    QString port_name;
    QByteArray request;
    int wait_timeout = 0;
    QMutex mutex;
    QWaitCondition condition;
    bool quit = false;
};

*/

class Dome: public QObject {
    Q_OBJECT
private:
    constexpr static unsigned int REFRESH = 300;
    const static Request RequestBasic, RequestEnv, RequestShaft;
    const static Command CommandNoOp;
    const static Command CommandOpenCover, CommandCloseCover;
    const static Command CommandFanOn, CommandFanOff;
    const static Command CommandIIOn, CommandIIOff;
    const static Command CommandHotwireOn, CommandHotwireOff;
    const static Command CommandResetSlave;

    unsigned char address;
    QDateTime last_received;
    std::default_random_engine generator;

    QSerialPort *m_serial_port;
    QTimer *refresh_timer;

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

    /* maps for storing states and their associated information
        (code, verbose name, further properties may be added as needed) */
    const static QMap<CoverState, State> Cover;
    const static QMap<TernaryState, State> Ternary;

    CoverState cover_state = CoverState::CLOSED;
    TernaryState heating_state = TernaryState::UNKNOWN;
    TernaryState intensifier_state = TernaryState::UNKNOWN;
    TernaryState fan_state = TernaryState::UNKNOWN;

    Dome();
    ~Dome();

    void reset_serial_port(const QString& port);

    const QDateTime& get_last_received(void) const;
    const QString serial_port_info(void) const;

    const DomeStateS& state_S(void) const;
    const DomeStateT& state_T(void) const;
    const DomeStateZ& state_Z(void) const;

    void send_command(const Command& command) const;
    void send_request(const Request& request) const;
    void send(const QByteArray& message) const;

    QJsonObject json(void) const;

public slots:
    void open_cover(bool manual);
    void close_cover(bool manual);



    void toggle_lens_heating(void);
    void toggle_fan(void);
    void toggle_intensifier(void);

    void request_status(void);
    void process_response(void);
    void process_message(const QByteArray &message);
    void handle_error(QSerialPort::SerialPortError error);

signals:
    void read_timeout(void) const;
    void write_timeout(void) const;
    void response_received(const QByteArray& response) const;

    void state_updated_S(void) const;
    void state_updated_T(void) const;
    void state_updated_Z(void) const;
};

#endif // DOMEMANAGER_H
