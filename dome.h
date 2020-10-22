#include <random>

#include <QSerialPort>
#include <QTimer>
#include <QJsonObject>

#ifndef DOMEMANAGER_H
#define DOMEMANAGER_H

#include "forward.h"
#include "domestate.h"

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
    constexpr static unsigned int REFRESH = 500;
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

    QSerialPort *serial_port;
    QTimer *refresh_timer;

    QByteArray buffer;

    void update_status_basic(void);
    void update_status_environment(void);
    void update_status_shaft(void);

    void process_response_S(const QByteArray &response);
    void process_response_T(const QByteArray &response);
    void process_response_Z(const QByteArray &response);

    DomeStateS state_S;
    DomeStateT state_T;
    DomeStateZ state_Z;
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

    void init_serial_port(const QString& port);

    const QDateTime& get_last_received(void) const;

    const DomeStateS& get_state_S(void) const;
    const DomeStateT& get_state_T(void) const;
    const DomeStateZ& get_state_Z(void) const;

    void send_command(const Command& command) const;
    void send_request(const Request& request) const;
    void send(const QByteArray& message) const;

    QJsonObject json(void) const;

public slots:
    void open_cover(void);
    void close_cover(void);

    void toggle_fan(void);
    void toggle_heating(void);
    void toggle_intensifier(void);

    void request_status(void);
    void process_response(void);
    void handle_error(QSerialPort::SerialPortError error);

signals:
    void read_timeout(void) const;
    void write_timeout(void) const;
    void response_received(const QByteArray& response) const;
};

#endif // DOMEMANAGER_H
