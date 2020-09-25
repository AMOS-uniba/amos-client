#include <random>

#include <QSerialPort>
#include <QJsonObject>

#ifndef DOMEMANAGER_H
#define DOMEMANAGER_H

enum CoverState {
    COVER_OPEN,
    COVER_OPENING,
    COVER_CLOSING,
    COVER_CLOSED,
    COVER_SAFETY,
};

enum HeatingState {
    HEATING_ON,
    HEATING_OFF,
    HEATING_PROBLEM,
};

enum Command {
    COMMAND_NOP,                       // do nothing, just for testing
    COMMAND_COVER_OPEN,                // open the cover
    COMMAND_COVER_CLOSE,               // close the cover
    COMMAND_FAN_ON,                    // turn on the fan
    COMMAND_FAN_OFF,                   // turn off the fan
    COMMAND_II_ON,                     // turn on image intensifier
    COMMAND_II_OFF,                    // turn off image intensifier
    COMMAND_SW_RESET                   // perform software reset
};

static QMap<Command, QChar> Commands = {
    {COMMAND_NOP, '\x00'},
    {COMMAND_COVER_OPEN, '\x01'},
    {COMMAND_COVER_CLOSE, '\x02'},
    {COMMAND_FAN_ON, '\x05'},
    {COMMAND_FAN_OFF, '\x06'},
    {COMMAND_II_ON, '\x07'},
    {COMMAND_II_OFF, '\x08'},
    {COMMAND_SW_RESET, '\x0b'}
};

static QMap<CoverState, QString> cover_code = {
    {CoverState::COVER_OPEN, "O"},
    {CoverState::COVER_OPENING, ">"},
    {CoverState::COVER_CLOSED, "C"},
    {CoverState::COVER_CLOSING, "<"},
    {CoverState::COVER_SAFETY, "S"},
};

static QMap<HeatingState, QString> heating_code = {
    {HeatingState::HEATING_OFF, "0"},
    {HeatingState::HEATING_ON, "1"},
    {HeatingState::HEATING_PROBLEM, "P"},
};


class DomeManager {
private:
    const QString& get_cover_code(void) const;
    const QString& get_heating_code(void) const;

    std::default_random_engine generator;
public:
    double temperature;
    double pressure;
    double humidity;

    unsigned int cover_position = 0;
    bool heating = false;

    CoverState cover_state = CoverState::COVER_CLOSED;
    HeatingState heating_state = HeatingState::HEATING_OFF;

    DomeManager();

    QDateTime last_received;
    void fake_env_data(void);

    void send_command(const Command& command) const;
    QJsonObject json(void) const;
};

#endif // DOMEMANAGER_H
