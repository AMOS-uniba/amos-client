#ifndef DOMESTATE_H
#define DOMESTATE_H

#include <QString>

#include "forward.h"

class DomeState {
private:
    QDateTime m_timestamp;
protected:
    static float deciint(const QByteArray &chunk);
    bool m_valid;
public:
    DomeState(void);
    const QDateTime& timestamp(void) const;
    float age(void) const;
    void set_valid(void);
    bool is_valid(void) const;
    virtual QJsonValue json(void) const = 0;
};



class DomeStateS: public DomeState {
private:
    unsigned char m_basic;
    unsigned char m_env;
    unsigned char m_errors;
    unsigned int m_time_alive;
public:
    DomeStateS(void);
    DomeStateS(const QByteArray &response);

    bool servo_moving(void) const;
    bool servo_direction(void) const;
    bool dome_open_sensor_active(void) const;
    bool dome_closed_sensor_active(void) const;
    bool lens_heating_active(void) const;
    bool camera_heating_active(void) const;
    bool intensifier_active(void) const;
    bool fan_active(void) const;

    bool rain_sensor_active(void) const;
    bool light_sensor_active(void) const;
    bool computer_power_sensor_active(void) const;
    bool cover_safety_position(void) const;
    bool servo_blocked(void) const;

    bool error_t_lens(void) const;
    bool error_SHT31(void) const;
    bool emergency_closing_light(void) const;
    bool error_watchdog_reset(void) const;
    bool error_brownout_reset(void) const;
    bool error_computer_power(void) const;
    bool error_t_CPU(void) const;
    bool emergency_closing_rain(void) const;

    unsigned int time_alive(void) const;

    QByteArray full_text(void) const;
    QJsonValue json(void) const override;
};



class DomeStateT: public DomeState {
private:
    float m_temp_lens, m_temp_cpu, m_temp_sht, m_humi_sht;
public:
    DomeStateT(void);
    DomeStateT(const QByteArray &response);

    float temperature_lens(void) const;
    float temperature_cpu(void) const;
    float temperature_sht(void) const;
    float humidity_sht(void) const;

    QJsonValue json(void) const override;
};



class DomeStateZ: public DomeState {
private:
    unsigned short int m_shaft_position;
public:
    DomeStateZ(void);
    DomeStateZ(const QByteArray &response);

    unsigned short int shaft_position(void) const;

    QJsonValue json(void) const override;
};

#endif // DOMESTATE_H
