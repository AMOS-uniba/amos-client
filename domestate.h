#ifndef DOMESTATE_H
#define DOMESTATE_H

#include <QString>

#include "forward.h"

class DomeState {
protected:
    unsigned char state;
public:
    DomeState(unsigned char byte);
    virtual QByteArray full_text(void) const = 0;
};


class DomeStateBasic: public DomeState {
public:
    bool servo_moving(void) const;
    bool servo_direction(void) const;
    bool dome_open_sensor_active(void) const;
    bool dome_closed_sensor_active(void) const;
    bool lens_heating_active(void) const;
    bool camera_heating_active(void) const;
    bool intensifier_active(void) const;
    bool fan_active(void) const;

    QByteArray full_text(void) const override;
};

class DomeStateEnv: public DomeState {
public:
    DomeStateEnv(unsigned char byte);
    bool rain_sensor_active(void) const;
    bool light_sensor_active(void) const;
    bool computer_power_sensor_active(void) const;
    bool cover_safety_position(void) const;
    bool servo_blocked(void) const;

    QByteArray full_text(void) const override;
};

class DomeStateError: public DomeState {
public:
    bool error_t_lens(void) const;
    bool error_SHT31(void) const;
    bool emergency_closing_light(void) const;
    bool error_watchdog_reset(void) const;
    bool error_brownout_reset(void) const;
    bool error_computer_power(void) const;
    bool error_t_CPU(void) const;
    bool emergency_closing_rain(void) const;

    QByteArray full_text(void) const override;
};

#endif // DOMESTATE_H
