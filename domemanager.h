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
};

enum class TernaryState {
    ON,
    OFF,
    UNKNOWN,
};

enum class Command {
    NOP,                       // do nothing, just for testing
    COVER_OPEN,                // open the cover
    COVER_CLOSE,               // close the cover
    FAN_ON,                    // turn on the fan
    FAN_OFF,                   // turn off the fan
    II_ON,                     // turn on image intensifier
    II_OFF,                    // turn off image intensifier
    SW_RESET,                  // perform software reset
};


struct State {
    char code;
    QString display_name;
};

struct CommandInfo {
    char code;
    QString display_name;
};


class DomeManager {
private:
    QDateTime last_received;
    std::default_random_engine generator;

    const static QMap<CoverState, State> Cover;
    const static QMap<TernaryState, State> Ternary;
    const static QMap<Command, CommandInfo> Commands;

    char cover_code(void) const;
    char ternary_code(TernaryState state) const;
    char fan_code(void) const;
    char heating_code(void) const;
    char intensifier_code(void) const;

    char command_code(Command command) const;
    QString command_name(Command command) const;
    QString ternary_name(TernaryState state) const;

public:
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
    QString fan_state_name(void) const;
    QString intensifier_state_name(void) const;

    void send_command(const Command& command) const;

    QJsonObject json(void) const;
};

#endif // DOMEMANAGER_H
