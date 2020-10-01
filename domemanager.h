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
};

enum class HeatingState {
    ON,
    OFF,
    UNKNOWN,
};

enum class IntensifierState {
    ON,
    OFF,
    UNKNOWN,
};

enum Command {
    NOP,                       // do nothing, just for testing
    COVER_OPEN,                // open the cover
    COVER_CLOSE,               // close the cover
    FAN_ON,                    // turn on the fan
    FAN_OFF,                   // turn off the fan
    II_ON,                     // turn on image intensifier
    II_OFF,                    // turn off image intensifier
    SW_RESET,                  // perform software reset
};


class DomeManager {
private:
    QDateTime last_received;
    std::default_random_engine generator;

    const static QMap<Command, QChar> Commands;
    const static QMap<CoverState, QString> CoverCode;
    const static QMap<HeatingState, QString> HeatingCode;
    const static QMap<IntensifierState, QString> IntensifierCode;

    const QString& get_cover_code(void) const;
    const QString& get_heating_code(void) const;
    const QString& get_intensifier_code(void) const;
public:
    double temperature;
    double pressure;
    double humidity;

    unsigned int cover_position = 0;

    CoverState cover_state = CoverState::CLOSED;
    HeatingState heating_state = HeatingState::OFF;
    IntensifierState intensifier_state = IntensifierState::OFF;

    DomeManager();

    void fake_env_data(void);
    void fake_gizmo_data(void);

    const QDateTime& get_last_received(void) const;

    void send_command(const Command& command) const;
    QJsonObject json(void) const;
};

#endif // DOMEMANAGER_H
