#include <random>

#include <QSerialPort>
#include <QTimer>
#include <QJsonObject>

#ifndef DOMEMANAGER_H
#define DOMEMANAGER_H

#include "forward.h"
#include "domestate.h"
#include "serialbuffer.h"
#include "serialthread.h"

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
*/

class Dome: public QObject {
    Q_OBJECT
private:
    constexpr static unsigned int REFRESH = 2000;

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

    DomeStateS state_S;
    DomeStateT state_T;
    DomeStateZ state_Z;

    SerialThread *m_serial_thread;
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

    void send_command(const Command& command);
    void send_request(const Request& request);
    void send(const QByteArray& message);

    QJsonObject json(void) const;

public slots:
    void open_cover(void);
    void close_cover(void);


    void toggle_lens_heating(void);
    void toggle_fan(void);
    void toggle_intensifier(void);

    void request_status(void);
    void process_message(const QByteArray &message);
    void process_timeout(const QString &timeout);
    void process_error(const QString &error);

signals:
    void read_timeout(void) const;
    void write_timeout(void) const;
    void response_received(const QByteArray& response) const;

    void state_updated_S(void) const;
    void state_updated_T(void) const;
    void state_updated_Z(void) const;
};

#endif // DOMEMANAGER_H
