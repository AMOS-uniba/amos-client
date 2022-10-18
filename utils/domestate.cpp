#include "domestate.h"

extern EventLogger logger;


DomeState::DomeState(void):
    m_timestamp(QDateTime::currentDateTimeUtc()),
    m_valid(false) {}

float DomeState::deciint(const QByteArray & chunk) {
   short int x;
   memcpy(&x, chunk.data(), 2);
   return (float) (x / 10.0);
}

bool DomeState::is_valid(void) const {
    logger.debug(Concern::SerialPort, QString("State age is %1").arg(this->age()));
    return (this->m_valid && (this->age() < 2.0));
}

float DomeState::age(void) const {
    return this->m_timestamp.msecsTo(QDateTime::currentDateTimeUtc()) / 1000.0;
}


DomeStateS::DomeStateS(void):
    DomeState(),
    m_basic(0),
    m_env(0),
    m_errors(0),
    m_time_alive(0) {}

DomeStateS::DomeStateS(const QByteArray & response):
    DomeState()
{
    if (response.length() != 8) {
        throw InvalidState(QString("Wrong S-state length %1").arg(response.length()));
    }
    if ((response[0] != 'S') && (response[0] != 'C')) {
        throw InvalidState(QString("Invalid first char of S state message '%1'").arg(QString(QChar(response[0]))));
    }

    this->m_basic     = response[1];
    this->m_env       = response[2];
    this->m_errors    = response[3];
    memcpy(&this->m_time_alive, response.mid(4, 4), 4);

    this->m_valid     = true;
    logger.debug(Concern::SerialPort, QString("S state received: %1").arg(QString(this->full_text())));
}

// Return textual representation of the state (three bytes as received, char for true, dash for false)
QByteArray DomeStateS::full_text(void) const {
    QByteArray result(26, '-');
    result[     0] = this->servo_moving()                        ? 'M' : '-';
    result[     1] = this->servo_direction()                     ? 'O' : '-';
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
    result[18 + 5] = this->error_master_power()                  ? 'P' : '-';
    result[18 + 6] = this->error_t_CPU()                         ? 'C' : '-';
    result[18 + 7] = this->emergency_closing_rain()              ? 'R' : '-';
    return result;
}

// Return a JSON summary of the state
QJsonValue DomeStateS::json() const {
    if (this->is_valid()) {
        return QJsonValue(QString(this->full_text()));
    } else {
        return QJsonValue(QJsonValue::Null);
    }
}

DomeStateT::DomeStateT(void):
    DomeState(),
    m_temp_lens(0),
    m_temp_CPU(0),
    m_temp_SHT31(0),
    m_humi_SHT31(0) {}

/**
 * @brief DomeStateT::DomeStateT
 * Construct a new DomeStateT from a response from the dome
 * @throws InvalidState
 */
DomeStateT::DomeStateT(const QByteArray & response):
    DomeState()
{
    if (response.length() != 9) {
        throw InvalidState(QString("Wrong T-state length %1").arg(response.length()));
    }
    if (response[0] != 'T') {
        throw InvalidState(QString("Invalid first char of T state message '%1'").arg(QString(QChar(response[0]))));
    }

    this->m_temp_lens  = DomeStateT::deciint(response.mid(1, 2));
    this->m_temp_CPU   = DomeStateT::deciint(response.mid(3, 2));
    this->m_temp_SHT31 = DomeStateT::deciint(response.mid(5, 2));
    this->m_humi_SHT31 = DomeStateT::deciint(response.mid(7, 2));

    this->m_valid      = true;
    logger.debug(Concern::SerialPort, QString("T state received: %1 %2 %3 %4")
                 .arg(this->m_temp_lens, 4, 'f', 1)
                 .arg(this->m_temp_CPU, 4, 'f', 1)
                 .arg(this->m_temp_SHT31, 4, 'f', 1)
                 .arg(this->m_humi_SHT31, 4, 'f', 1));
}

float DomeStateT::temperature_lens(void) const { return this->m_temp_lens; }
float DomeStateT::temperature_CPU(void) const  { return this->m_temp_CPU; }
float DomeStateT::temperature_sht(void) const  { return this->m_temp_SHT31; }
float DomeStateT::humidity_sht(void) const     { return this->m_humi_SHT31; }

QJsonValue DomeStateT::json(void) const {
    if (this->is_valid()) {
        return QJsonObject {
            {"t_lens", this->temperature_lens()},
            {"t_cpu", this->temperature_CPU()},
            {"t_sht", this->temperature_sht()},
            {"h_sht", this->humidity_sht()},
        };
    } else {
        return QJsonValue(QJsonValue::Null);
    }
}



DomeStateZ::DomeStateZ(void):
    DomeState(),
    m_shaft_position(0) {}

DomeStateZ::DomeStateZ(const QByteArray & response):
    DomeState()
{
#if OLD_PROTOCOL
    if (response.length() != 9) {
        throw InvalidState(QString("Wrong Z-state length %1").arg(response.length()));
    }
    if (response[0] != 'W') {
        throw InvalidState(QString("Invalid first char of W state message '%1'").arg(QString(QChar(response[0]))));
    }
    memcpy(&this->m_shaft_position, response.mid(7, 2).data(), 2);
#else
    if (response.length() != 3) {
        throw InvalidState(QString("Wrong Z-state length %1").arg(response.length()));
    }
    if (response[0] != 'Z') {
        throw InvalidState(QString("Invalid first char of Z state message '%1'").arg(QString(QChar(response[0]))));
    }
    memcpy(&this->m_shaft_position, response.mid(1, 2).data(), 2);
#endif

    this->m_valid = true;
    logger.debug(Concern::SerialPort, QString("Z state received: %1").arg(this->m_shaft_position));
}

short int DomeStateZ::shaft_position(void) const {
    return this->m_shaft_position;
}

QJsonValue DomeStateZ::json(void) const {
    if (this->is_valid()) {
        return QJsonObject {{"sp", this->shaft_position()}};
    } else {
        return QJsonValue(QJsonValue::Null);
    }
}
