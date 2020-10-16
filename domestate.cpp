#include "domestate.h"

extern Log logger;

DomeState::DomeState(unsigned char byte): state(byte) {}

bool DomeStateBasic::servo_moving(void) const               { return this->state & 0x01; }
bool DomeStateBasic::servo_direction(void) const            { return this->state & 0x02; }
bool DomeStateBasic::dome_open_sensor_active(void) const    { return this->state & 0x04; }
bool DomeStateBasic::dome_closed_sensor_active(void) const  { return this->state & 0x08; }
bool DomeStateBasic::lens_heating_active(void) const        { return this->state & 0x10; }
bool DomeStateBasic::camera_heating_active(void) const      { return this->state & 0x20; }
bool DomeStateBasic::intensifier_active(void) const         { return this->state & 0x40; }
bool DomeStateBasic::fan_active(void) const                 { return this->state & 0x80; }

QByteArray DomeStateBasic::full_text(void) const {
    QByteArray result(8, '-');
    result[0] = this->servo_moving()                        ? '1' : '0';
    result[1] = this->servo_direction()                     ? 'V' : 'O';
    result[2] = this->dome_open_sensor_active()             ? 'O' : 'o';
    result[3] = this->dome_closed_sensor_active()           ? 'C' : 'c';
    result[4] = this->lens_heating_active()                 ? 'H' : 'h';
    result[5] = this->camera_heating_active()               ? 'C' : 'c';
    result[6] = this->intensifier_active()                  ? 'I' : 'i';
    result[7] = this->fan_active()                          ? 'F' : 'f';
    return result;
}

bool DomeStateEnv::rain_sensor_active(void) const           { return this->state & 0x01; }
bool DomeStateEnv::light_sensor_active(void) const          { return this->state & 0x02; }
bool DomeStateEnv::computer_power_sensor_active(void) const { return this->state & 0x04; }
bool DomeStateEnv::cover_safety_position(void) const        { return this->state & 0x20; }
bool DomeStateEnv::servo_blocked(void) const                { return this->state & 0x80; }

QByteArray DomeStateEnv::full_text(void) const {
    QByteArray result(8, '-');
    result[0] = this->rain_sensor_active()                  ? 'R' : '0';
    result[1] = this->light_sensor_active()                 ? 'L' : '0';
    result[2] = this->computer_power_sensor_active()        ? 'P' : '0';
    result[5] = this->cover_safety_position()               ? 'S' : '0';
    result[7] = this->servo_blocked()                       ? 'B' : '0';
    return result;
}

bool DomeStateError::error_t_lens(void) const               { return this->state & 0x01; }
bool DomeStateError::error_SHT31(void) const                { return this->state & 0x02; }
bool DomeStateError::emergency_closing_light(void) const    { return this->state & 0x04; }
bool DomeStateError::error_watchdog_reset(void) const       { return this->state & 0x08; }
bool DomeStateError::error_brownout_reset(void) const       { return this->state & 0x10; }
bool DomeStateError::error_computer_power(void) const       { return this->state & 0x20; }
bool DomeStateError::error_t_CPU(void) const                { return this->state & 0x40; }
bool DomeStateError::emergency_closing_rain(void) const     { return this->state & 0x80; }

QByteArray DomeStateError::full_text(void) const {
    QByteArray result(8, '-');
    result[0] = this->error_t_lens()                        ? 'T' : '-';
    result[1] = this->error_SHT31()                         ? 'S' : '-';
    result[2] = this->emergency_closing_light()             ? 'L' : '-';
    result[3] = this->error_watchdog_reset()                ? 'W' : '-';
    result[4] = this->error_brownout_reset()                ? 'B' : '-';
    result[5] = this->error_computer_power()                ? 'P' : '-';
    result[6] = this->error_t_CPU()                         ? 'C' : '-';
    result[7] = this->emergency_closing_rain()              ? 'R' : '-';
    return result;
}
