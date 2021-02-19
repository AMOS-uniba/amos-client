#include "qdomeinfo.h"
#include "ui_qdomeinfo.h"

QColor QDomeInfo::temperature_colour(double temperature) {
    float h = 0, s = 0, v = 0;
    if (temperature < 0.0) {
        h = 180 - 4.0 * temperature;
        s = (1 + temperature / 50.0) * 255;
        v = 200;
    } else {
        if (temperature < 15) {
            h = 180 - 6.0 * temperature;
        } else {
            h = 90 - (4.5 * (temperature - 15));
        }
        if (h < 0) {
            h += 360;
        }
        s = 255;
        v = 160;
    }
    return QColor::fromHsv(h, s, v);
}


QDomeInfo::QDomeInfo(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::QDomeInfo)
{
    this->ui->setupUi(this);

    this->ui->dl_time_alive->set_title("Time alive");
    this->ui->dl_time_alive->set_value_formatter(
        [](double value) -> QString {
            return MainWindow::format_duration((unsigned int) value);
        }
    );

    this->ui->bl_servo_moving->set_title("Servo moving");
    this->ui->bl_servo_direction->set_title("Servo direction");
    this->ui->bl_open_dome_sensor->set_title("Open dome sensor");
    this->ui->bl_closed_dome_sensor->set_title("Closed dome sensor");
    this->ui->bl_safety->set_title("Safety position");
    this->ui->bl_servo_blocked->set_title("Servo blocked");

    this->ui->cl_lens_heating->set_title("Lens heating");
    this->ui->cl_camera_heating->set_title("Camera heating");
    this->ui->cl_fan->set_title("Fan");
    this->ui->cl_fan->set_formatters(Qt::green, Qt::black, "on", "off");

    this->ui->bl_t_lens->set_title("Lens temperature");
    this->ui->bl_t_CPU->set_title("CPU temperature");
    this->ui->bl_t_SHT31->set_title("Ambient temperature");
    this->ui->bl_h_SHT31->set_title("Ambient humidity");
    this->ui->bl_rain_sensor->set_title("Rain sensor");
    this->ui->bl_light_sensor->set_title("Light sensor");
    this->ui->bl_master_power->set_title("Master power");

    this->ui->bl_error_t_lens->set_title("Lens temperature");
    this->ui->bl_error_t_lens->set_formatters(Qt::red, Qt::black, "error", "ok");
    this->ui->bl_error_t_SHT31->set_title("Environment temperature");
    this->ui->bl_error_t_SHT31->set_formatters(Qt::red, Qt::black, "error", "ok");
    this->ui->bl_error_light->set_title("Emergency closing (light)");
    this->ui->bl_error_light->set_formatters(Qt::red, Qt::black, "light", "no light");
    this->ui->bl_error_watchdog->set_title("Watchdog reset");
    this->ui->bl_error_watchdog->set_formatters(Qt::red, Qt::black, "reset", "ok");
    this->ui->bl_error_brownout->set_title("Brownout reset");
    this->ui->bl_error_brownout->set_formatters(Qt::red, Qt::black, "reset", "ok");
    this->ui->bl_error_master->set_title("Master power");
    this->ui->bl_error_master->set_formatters(Qt::red, Qt::black, "failed", "ok");
    this->ui->bl_error_t_CPU->set_title("CPU temperature");
    this->ui->bl_error_t_CPU->set_formatters(Qt::red, Qt::black, "error", "ok");
    this->ui->bl_error_rain->set_title("Emergency closing (rain)");
    this->ui->bl_error_rain->set_formatters(Qt::red, Qt::black, "closed", "ok");

    this->connect(this, &QDomeInfo::cover_moved, this->ui->picture, &DomeWidget::set_cover_position);
    this->connect(this, &QDomeInfo::cover_open, this->ui->picture, &DomeWidget::set_cover_maximum);
    this->connect(this, &QDomeInfo::cover_open, this->ui->pb_cover, &QProgressBar::setMaximum);
    this->connect(this, &QDomeInfo::cover_closed, this->ui->picture, &DomeWidget::set_cover_minimum);
    this->connect(this, &QDomeInfo::cover_closed, this->ui->pb_cover, &QProgressBar::setMinimum);
}

QDomeInfo::~QDomeInfo() {
    delete ui;
}

void QDomeInfo::set_formatters(double humidity_limit_lower, double humidity_limit_upper) {
    ValueFormatter<double> temp_value_formatter = [=](double value) -> QString { return QString("%1 °C").arg(value); };
    ValueFormatter<double> humi_value_formatter = [=](double value) -> QString { return QString("%1 %%").arg(value); };

    this->ui->bl_servo_moving->set_formatters(Qt::green, Qt::black, "moving", "not moving");
    this->ui->bl_servo_direction->set_formatters(Qt::black, Qt::black, "opening", "closing");
    this->ui->bl_open_dome_sensor->set_formatters(Qt::green, Qt::black, "active", "not active");
    this->ui->bl_closed_dome_sensor->set_formatters(Qt::green, Qt::black, "active", "not active");
    this->ui->bl_safety->set_formatters(Qt::blue, Qt::black, "safety", "no");
    this->ui->bl_servo_blocked->set_formatters(Qt::red, Qt::black, "blocked", "no");

    this->ui->bl_t_lens->set_value_formatter(temp_value_formatter);
    this->ui->bl_t_lens->set_colour_formatter(QDomeInfo::temperature_colour);
    this->ui->bl_t_CPU->set_value_formatter(temp_value_formatter);
    this->ui->bl_t_CPU->set_colour_formatter(QDomeInfo::temperature_colour);
    this->ui->bl_t_SHT31->set_value_formatter(temp_value_formatter);
    this->ui->bl_t_SHT31->set_colour_formatter(QDomeInfo::temperature_colour);
    this->ui->bl_h_SHT31->set_value_formatter(humi_value_formatter);
    this->ui->bl_h_SHT31->set_colour_formatter([=](double humidity) -> QColor {
        return (humidity < humidity_limit_lower) ? Qt::black :
            (humidity > humidity_limit_upper) ? Qt::red : Qt::blue;
    });
    this->ui->bl_rain_sensor->set_formatters(Qt::blue, Qt::black, "raining", "not raining");
    this->ui->bl_light_sensor->set_formatters(Qt::red, Qt::black, "light", "no light");
    this->ui->bl_master_power->set_formatters(Qt::black, Qt::red, "powered", "not powered");

    this->ui->cl_lens_heating->set_formatters(Qt::green, Qt::black, "on", "off");
    this->ui->cl_camera_heating->set_formatters(Qt::green, Qt::black, "on", "off");
    this->ui->cl_ii->set_title("Image intensifier");
    this->ui->cl_ii->set_formatters(Qt::green, Qt::black, "on", "off");
}

void QDomeInfo::display_basic_data(const DomeStateS state) {
    bool valid = state.is_valid();

    this->ui->dl_time_alive->set_valid(valid);

    this->ui->bl_servo_moving->set_valid(valid);
    this->ui->bl_servo_direction->set_valid(valid);
    this->ui->bl_open_dome_sensor->set_valid(valid);
    this->ui->bl_closed_dome_sensor->set_valid(valid);
    this->ui->bl_safety->set_valid(valid);
    this->ui->bl_servo_blocked->set_valid(valid);

    this->ui->cl_lens_heating->set_valid(valid);
    this->ui->cl_camera_heating->set_valid(valid);
    this->ui->cl_fan->set_valid(valid);
    this->ui->cl_ii->set_valid(valid);

    this->ui->bl_error_t_lens->set_valid(valid);
    this->ui->bl_error_t_SHT31->set_valid(valid);
    this->ui->bl_error_light->set_valid(valid);
    this->ui->bl_error_watchdog->set_valid(valid);
    this->ui->bl_error_brownout->set_valid(valid);
    this->ui->bl_error_master->set_valid(valid);
    this->ui->bl_error_t_CPU->set_valid(valid);
    this->ui->bl_error_rain->set_valid(valid);

    // Set values
    this->ui->dl_time_alive->set_value(state.time_alive());

    this->ui->bl_servo_moving->set_value(state.servo_moving());
    this->ui->bl_servo_direction->set_value(state.servo_direction());
    this->ui->bl_open_dome_sensor->set_value(state.dome_open_sensor_active());
    this->ui->bl_closed_dome_sensor->set_value(state.dome_closed_sensor_active());
    this->ui->bl_safety->set_value(state.cover_safety_position());
    this->ui->bl_servo_blocked->set_value(state.servo_blocked());

    this->ui->cl_lens_heating->set_value(state.lens_heating_active());
    this->ui->cl_camera_heating->set_value(state.camera_heating_active());
    this->ui->cl_fan->set_value(state.fan_active());
    this->ui->cl_ii->set_value(state.intensifier_active());

    this->ui->bl_error_t_lens->set_value(state.error_t_lens());
    this->ui->bl_error_t_SHT31->set_value(state.error_SHT31());
    this->ui->bl_error_light->set_value(state.emergency_closing_light());
    this->ui->bl_error_watchdog->set_value(state.error_watchdog_reset());
    this->ui->bl_error_brownout->set_value(state.error_brownout_reset());
    this->ui->bl_error_master->set_value(state.error_master_power());
    this->ui->bl_error_t_CPU->set_value(state.error_t_CPU());
    this->ui->bl_error_rain->set_value(state.emergency_closing_rain());
}

void QDomeInfo::display_env_data(const DomeStateT state) {
    this->ui->bl_t_lens->set_value(state.temperature_lens());
    this->ui->bl_error_t_CPU->set_value(state.temperature_CPU());
    this->ui->bl_t_SHT31->set_value(state.temperature_sht());
    this->ui->bl_h_SHT31->set_value(state.humidity_sht());
}

void QDomeInfo::display_shaft_data(const DomeStateZ state) {
}
