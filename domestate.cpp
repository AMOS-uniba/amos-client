#include "include.h"

extern Log logger;



DomeState::DomeState(void):
    m_timestamp(QDateTime::currentDateTimeUtc()),
    m_valid(false) {}

float DomeState::deciint(const QByteArray &chunk) {
   short int x;
   memcpy(&x, chunk.data(), 2);
   return (float) (x / 10.0);
}

bool DomeState::is_valid(void) const {
    return (this->m_valid && (this->age() < 5));
}

const QDateTime& DomeState::timestamp(void) const {
    return this->m_timestamp;
}

float DomeState::age(void) const {
    return this->m_timestamp.msecsTo(QDateTime::currentDateTimeUtc()) / 1000.0;
}


DomeStateS::DomeStateS(void): DomeState() {
    this->m_basic = 0;
    this->m_env = 0;
    this->m_errors = 0;
    this->m_time_alive = 0;
}

DomeStateS::DomeStateS(const QByteArray &response) {
    if (response.length() != 8) {
        throw InvalidState(QString("Wrong S-state length %1").arg(response.length()));
    }
    this->m_basic     = response[1];
    this->m_env       = response[2];
    this->m_errors    = response[3];
    this->m_valid     = true;
    memcpy(&this->m_time_alive, response.mid(4, 4), 4);

    logger.debug(QString("S state received: %1").arg(QString(this->full_text())));
}

bool DomeStateS::servo_moving(void) const                   { return this->m_basic & 0x01; }
bool DomeStateS::servo_direction(void) const                { return this->m_basic & 0x02; }
bool DomeStateS::dome_open_sensor_active(void) const        { return this->m_basic & 0x04; }
bool DomeStateS::dome_closed_sensor_active(void) const      { return this->m_basic & 0x08; }
bool DomeStateS::lens_heating_active(void) const            { return this->m_basic & 0x10; }
bool DomeStateS::camera_heating_active(void) const          { return this->m_basic & 0x20; }
bool DomeStateS::intensifier_active(void) const             { return this->m_basic & 0x40; }
bool DomeStateS::fan_active(void) const                     { return this->m_basic & 0x80; }

bool DomeStateS::rain_sensor_active(void) const             { return this->m_env & 0x01; }
bool DomeStateS::light_sensor_active(void) const            { return this->m_env & 0x02; }
bool DomeStateS::computer_power_sensor_active(void) const   { return this->m_env & 0x04; }
bool DomeStateS::cover_safety_position(void) const          { return this->m_env & 0x20; }
bool DomeStateS::servo_blocked(void) const                  { return this->m_env & 0x80; }

bool DomeStateS::error_t_lens(void) const                   { return this->m_errors & 0x01; }
bool DomeStateS::error_SHT31(void) const                    { return this->m_errors & 0x02; }
bool DomeStateS::emergency_closing_light(void) const        { return this->m_errors & 0x04; }
bool DomeStateS::error_watchdog_reset(void) const           { return this->m_errors & 0x08; }
bool DomeStateS::error_brownout_reset(void) const           { return this->m_errors & 0x10; }
bool DomeStateS::error_computer_power(void) const           { return this->m_errors & 0x20; }
bool DomeStateS::error_t_CPU(void) const                    { return this->m_errors & 0x40; }
bool DomeStateS::emergency_closing_rain(void) const         { return this->m_errors & 0x80; }

unsigned int DomeStateS::time_alive(void) const             { return this->m_time_alive / 75; }

QByteArray DomeStateS::full_text(void) const {
    QByteArray result(26, '-');
    result[     0] = this->servo_moving()                        ? '1' : '0';
    result[     1] = this->servo_direction()                     ? 'O' : 'C';
    result[     2] = this->dome_open_sensor_active()             ? 'O' : '-';
    result[     3] = this->dome_closed_sensor_active()           ? 'C' : '-';
    result[     4] = this->lens_heating_active()                 ? 'H' : '-';
    result[     5] = this->camera_heating_active()               ? 'C' : '-';
    result[     6] = this->intensifier_active()                  ? 'I' : '-';
    result[     7] = this->fan_active()                          ? 'F' : '-';
    result[     8] = '|';
    result[ 9 + 0] = this->rain_sensor_active()                  ? 'R' : '-';
    result[ 9 + 1] = this->light_sensor_active()                 ? 'L' : '-';
    result[ 9 + 2] = this->computer_power_sensor_active()        ? 'P' : '-';
    result[ 9 + 5] = this->cover_safety_position()               ? 'S' : '-';
    result[ 9 + 7] = this->servo_blocked()                       ? 'B' : '-';
    result[    17] = '|';
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

QJsonValue DomeStateS::json() const {
    if (this->is_valid()) {
        return QJsonValue(QString(this->full_text()));
    } else {
        return QJsonValue(QJsonValue::Null);
    }
}



DomeStateT::DomeStateT(void): DomeState() {
    this->m_temp_lens = 0;
    this->m_temp_cpu = 0;
    this->m_temp_sht = 0;
    this->m_humi_sht = 0;
}

DomeStateT::DomeStateT(const QByteArray &response) {
    if (response.length() != 9) {
        throw InvalidState(QString("Wrong T-state length %1").arg(response.length()));
    }

    this->m_temp_lens  = DomeStateT::deciint(response.mid(1, 2));
    this->m_temp_cpu   = DomeStateT::deciint(response.mid(3, 2));
    this->m_temp_sht   = DomeStateT::deciint(response.mid(5, 2));
    this->m_humi_sht   = DomeStateT::deciint(response.mid(7, 2));
    this->m_valid      = true;
    logger.debug(QString("T state received: %1 %2 %3 %4").arg(this->m_temp_lens).arg(this->m_temp_cpu).arg(this->m_temp_sht).arg(this->m_humi_sht));
}

float DomeStateT::temperature_lens(void) const              { return this->m_temp_lens; }
float DomeStateT::temperature_cpu(void) const               { return this->m_temp_cpu; }
float DomeStateT::temperature_sht(void) const               { return this->m_temp_sht; }
float DomeStateT::humidity_sht(void) const                  { return this->m_humi_sht; }

QJsonValue DomeStateT::json(void) const {
    if (this->is_valid()) {
        return QJsonObject {
            {"t_lens", this->temperature_lens()},
            {"t_cpu", this->temperature_cpu()},
            {"t_sht", this->temperature_sht()},
            {"h_sht", this->humidity_sht()},
        };
    } else {
        return QJsonValue(QJsonValue::Null);
    }
}



DomeStateZ::DomeStateZ(void): DomeState() {
    this->m_shaft_position = 0;
}

DomeStateZ::DomeStateZ(const QByteArray &response) {
    if (response.length() != 3) {
        throw InvalidState(QString("Wrong Z-state length %1").arg(response.length()));
    }

    memcpy(&this->m_shaft_position, response.mid(1, 2).data(), 2);
    this->m_valid = true;
    logger.debug(QString("Z state received: %1").arg(this->m_shaft_position));
}

unsigned short int DomeStateZ::shaft_position(void) const {
    return this->m_shaft_position;
}

QJsonValue DomeStateZ::json(void) const {
    if (this->is_valid()) {
        return QJsonObject {{"sp", this->shaft_position()}};
    } else {
        return QJsonValue(QJsonValue::Null);
    }
}
