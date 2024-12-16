#ifndef DOMESTATE_H
#define DOMESTATE_H

#include <QString>
#include <QSerialPort>
#include <QDateTime>


class DomeState {
private:
    QDateTime m_timestamp;
protected:
    static float deciint(const QByteArray & chunk);
    bool m_valid;
    DomeState(void);
public:
    inline const QDateTime & timestamp(void) const { return this->m_timestamp; };
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
    DomeStateS(const QByteArray & response);

    inline bool servo_moving(void) const                    { return this->m_basic & 0x01; };
    inline bool servo_direction(void) const                 { return this->m_basic & 0x02; };
    inline bool dome_open_sensor_active(void) const         { return this->m_basic & 0x04; };
    inline bool dome_closed_sensor_active(void) const       { return this->m_basic & 0x08; };
    inline bool lens_heating_active(void) const             { return this->m_basic & 0x10; };
    inline bool camera_heating_active(void) const           { return this->m_basic & 0x20; };
    inline bool intensifier_active(void) const              { return this->m_basic & 0x40; };
    inline bool fan_active(void) const                      { return this->m_basic & 0x80; };

    inline bool rain_sensor_active(void) const              { return this->m_env & 0x01; };
    inline bool light_sensor_active(void) const             { return this->m_env & 0x02; };
#if PROTOCOL == 2015
    inline bool computer_power_sensor_active(void) const    { return this->m_env & 0x08; };
#elif PROTOCOL == 2020
    inline bool computer_power_sensor_active(void) const    { return this->m_env & 0x04; };
#endif
    inline bool cover_safety_position(void) const           { return this->m_env & 0x20; };
    inline bool servo_blocked(void) const                   { return this->m_env & 0x80; };

    inline bool error_t_lens(void) const                    { return this->m_errors & 0x01; };
    inline bool error_SHT31(void) const                     { return this->m_errors & 0x02; };
    inline bool emergency_closing_light(void) const         { return this->m_errors & 0x04; };
    inline bool error_watchdog_reset(void) const            { return this->m_errors & 0x08; };
    inline bool error_brownout_reset(void) const            { return this->m_errors & 0x10; };
    inline bool error_master_power(void) const              { return this->m_errors & 0x20; };
    inline bool error_t_CPU(void) const                     { return this->m_errors & 0x40; };
    inline bool emergency_closing_rain(void) const          { return this->m_errors & 0x80; };

    inline unsigned int time_alive(void) const              { return this->m_time_alive / 75; };

    QByteArray full_text(void) const;
    QJsonValue json(void) const override;
};



class DomeStateT: public DomeState {
private:
    float m_temp_lens, m_temp_CPU, m_temp_SHT31, m_humi_SHT31;
public:
    DomeStateT(void);
    DomeStateT(const QByteArray & response);

    float temperature_lens(void) const;
    float temperature_CPU(void) const;
    float temperature_sht(void) const;
    float humidity_sht(void) const;

    QJsonValue json(void) const override;
};



class DomeStateZ: public DomeState {
private:
    short int m_shaft_position;
public:
    DomeStateZ(void);
    DomeStateZ(const QByteArray & response);

    short int shaft_position(void) const;

    QJsonValue json(void) const override;
};

#endif // DOMESTATE_H
