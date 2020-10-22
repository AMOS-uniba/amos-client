#include "include.h"

extern Log logger;

DomeStateS::DomeStateS(void) {
    this->basic = 0;
    this->env = 0;
    this->errors = 0;
    this->t_alive = 0;
}

DomeStateS::DomeStateS(const QByteArray &response) {
    if (response.length() != 8) {
        throw InvalidState(QString("Wrong S-state length %1").arg(response.length()));
    }
    this->basic     = response[1];
    this->env       = response[2];
    this->errors    = response[3];
    QDataStream(response.mid(4, 4)) >> this->t_alive;
}

bool DomeStateS::servo_moving(void) const                   { return this->basic & 0x01; }
bool DomeStateS::servo_direction(void) const                { return this->basic & 0x02; }
bool DomeStateS::dome_open_sensor_active(void) const        { return this->basic & 0x04; }
bool DomeStateS::dome_closed_sensor_active(void) const      { return this->basic & 0x08; }
bool DomeStateS::lens_heating_active(void) const            { return this->basic & 0x10; }
bool DomeStateS::camera_heating_active(void) const          { return this->basic & 0x20; }
bool DomeStateS::intensifier_active(void) const             { return this->basic & 0x40; }
bool DomeStateS::fan_active(void) const                     { return this->basic & 0x80; }

bool DomeStateS::rain_sensor_active(void) const             { return this->env & 0x01; }
bool DomeStateS::light_sensor_active(void) const            { return this->env & 0x02; }
bool DomeStateS::computer_power_sensor_active(void) const   { return this->env & 0x04; }
bool DomeStateS::cover_safety_position(void) const          { return this->env & 0x20; }
bool DomeStateS::servo_blocked(void) const                  { return this->env & 0x80; }

bool DomeStateS::error_t_lens(void) const                   { return this->errors & 0x01; }
bool DomeStateS::error_SHT31(void) const                    { return this->errors & 0x02; }
bool DomeStateS::emergency_closing_light(void) const        { return this->errors & 0x04; }
bool DomeStateS::error_watchdog_reset(void) const           { return this->errors & 0x08; }
bool DomeStateS::error_brownout_reset(void) const           { return this->errors & 0x10; }
bool DomeStateS::error_computer_power(void) const           { return this->errors & 0x20; }
bool DomeStateS::error_t_CPU(void) const                    { return this->errors & 0x40; }
bool DomeStateS::emergency_closing_rain(void) const         { return this->errors & 0x80; }

unsigned int DomeStateS::time_alive(void) const             { return this->t_alive; }

QByteArray DomeStateS::full_text(void) const {
    QByteArray result(26, '-');
    result[     0] = this->servo_moving()                        ? '1' : '0';
    result[     1] = this->servo_direction()                     ? 'V' : 'O';
    result[     2] = this->dome_open_sensor_active()             ? 'O' : 'o';
    result[     3] = this->dome_closed_sensor_active()           ? 'C' : 'c';
    result[     4] = this->lens_heating_active()                 ? 'H' : 'h';
    result[     5] = this->camera_heating_active()               ? 'C' : 'c';
    result[     6] = this->intensifier_active()                  ? 'I' : 'i';
    result[     7] = this->fan_active()                          ? 'F' : 'f';

    result[ 9 + 0] = this->rain_sensor_active()                  ? 'R' : '0';
    result[ 9 + 1] = this->light_sensor_active()                 ? 'L' : '0';
    result[ 9 + 2] = this->computer_power_sensor_active()        ? 'P' : '0';
    result[ 9 + 5] = this->cover_safety_position()               ? 'S' : '0';
    result[ 9 + 7] = this->servo_blocked()                       ? 'B' : '0';

    result[18 + 0] = this->error_t_lens()                        ? 'T' : '-';
    result[18 + 1] = this->error_SHT31()                         ? 'S' : '-';
    result[18 + 2] = this->emergency_closing_light()             ? 'L' : '-';
    result[18 + 3] = this->error_watchdog_reset()                ? 'W' : '-';
    result[18 + 4] = this->error_brownout_reset()                ? 'B' : '-';
    result[18 + 5] = this->error_computer_power()                ? 'P' : '-';
    result[18 + 6] = this->error_t_CPU()                         ? 'C' : '-';
    result[18 + 7] = this->emergency_closing_rain()              ? 'R' : '-';
    return result;
}



DomeStateT::DomeStateT(void) {
    this->t_lens = 0;
    this->t_cpu = 0;
    this->t_sht = 0;
    this->h_sht = 0;
}

float DomeStateT::deciint(const QByteArray &chunk) {
   short int x;
   memcpy(&x, chunk.data(), 2);
   return (float) (x / 10.0);
}

DomeStateT::DomeStateT(const QByteArray &response) {
    if (response.length() != 9) {
        throw InvalidState(QString("Wrong T-state length %1").arg(response.length()));
    }

    this->t_lens  = DomeStateT::deciint(response.mid(1, 2));
    this->t_cpu   = DomeStateT::deciint(response.mid(3, 2));
    this->t_sht   = DomeStateT::deciint(response.mid(5, 2));
    this->h_sht   = DomeStateT::deciint(response.mid(7, 2));
}

float DomeStateT::temperature_lens(void) const              { return this->t_lens; }
float DomeStateT::temperature_cpu(void) const               { return this->t_cpu; }
float DomeStateT::temperature_sht(void) const               { return this->t_sht; }
float DomeStateT::humidity_sht(void) const                  { return this->h_sht; }



DomeStateZ::DomeStateZ(void) {
    this->s_pos = 0;
}

DomeStateZ::DomeStateZ(const QByteArray &response) {
    if (response.length() != 3) {
        throw InvalidState(QString("Wrong Z-state length %1").arg(response.length()));
    }

    memcpy(&this->s_pos, response.mid(1, 2).data(), 2);
//    this->s_pos = (unsigned short) response[1] + ((int) response[2]) * 256;
}

unsigned short int DomeStateZ::shaft_position(void) const {
    logger.error(QString("%1").arg(this->s_pos));
    return this->s_pos;
}
