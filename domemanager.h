#include <random>

#include <QSerialPort>
#include <QJsonObject>

#ifndef DOMEMANAGER_H
#define DOMEMANAGER_H

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

enum class Command {
    NOP,                        // do nothing, just for testing
    COVER_OPEN,                 // open the cover
    COVER_CLOSE,                // close the cover
    FAN_ON,                     // turn on the fan
    FAN_OFF,                    // turn off the fan
    II_ON,                      // turn on image intensifier
    II_OFF,                     // turn off image intensifier
    SW_RESET,                   // perform software reset
};

struct State {
    char code;
    QString display_name;
};

struct CommandInfo {
    char code;
    QString display_name;
};


class DomeManager: public QObject {
    Q_OBJECT
private:
    unsigned char address;
    QDateTime last_received;
    std::default_random_engine generator;

    QSerialPort *serial_port;
public:

    /* maps for storing states and their associated information
        (code, verbose name, further properties may be added as needed) */
    const static QMap<CoverState, State> Cover;
    const static QMap<TernaryState, State> Ternary;
    const static QMap<Command, CommandInfo> Commands;

    double temperature;
    double pressure;
    double humidity;

    unsigned int cover_position = 0;

    CoverState cover_state = CoverState::CLOSED;
    TernaryState heating_state = TernaryState::UNKNOWN;
    TernaryState intensifier_state = TernaryState::UNKNOWN;
    TernaryState fan_state = TernaryState::UNKNOWN;

    DomeManager();

    void fake_env_data(void);
    void fake_gizmo_data(void);
    const QDateTime& get_last_received(void) const;

    void open_cover(void);
    void close_cover(void);

    void send_command(const Command& command) const;
    void process_response(void);

    QJsonObject json(void) const;
};

#endif // DOMEMANAGER_H
