#include <random>

#include <QSerialPort>
#include <QTimer>
#include <QJsonObject>

#ifndef DOMEMANAGER_H
#define DOMEMANAGER_H

#include "forward.h"

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


class Dome: public QObject {
    Q_OBJECT
private:
    constexpr static unsigned int REFRESH = 3000;
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

    void update_status_basic(void);
    void update_status_environment(void);
    void update_status_shaft(void);
public:

    /* maps for storing states and their associated information
        (code, verbose name, further properties may be added as needed) */
    const static QMap<CoverState, State> Cover;
    const static QMap<TernaryState, State> Ternary;

    double temperature;
    double pressure;
    double humidity;

    unsigned int cover_position = 0;

    CoverState cover_state = CoverState::CLOSED;
    TernaryState heating_state = TernaryState::UNKNOWN;
    TernaryState intensifier_state = TernaryState::UNKNOWN;
    TernaryState fan_state = TernaryState::UNKNOWN;

    Dome();
    ~Dome();

    void init_serial_port(const QString& port);

    void fake_env_data(void);
    void fake_gizmo_data(void);
    const QDateTime& get_last_received(void) const;


    void open_cover(void);
    void close_cover(void);
    void send_command(const Command& command) const;
    void send_request(const Request& request) const;
    void send(const QByteArray& message) const;

    QJsonObject json(void) const;

public slots:
    void toggle_fan(void);
    void toggle_heating(void);
    void toggle_intensifier(void);

    void request_status(void);

signals:
    void read_timeout(void) const;
    void write_timeout(void) const;
    void response_received(const QByteArray& response) const;
};

#endif // DOMEMANAGER_H