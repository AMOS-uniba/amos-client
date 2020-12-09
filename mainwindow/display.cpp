#include "include.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QPushButton>

/* Display boolean bit `value` in `label`
   using string `on` with colour `colour_on` if true
   using string `off` with colour `colour_off` if false */
void MainWindow::display_S_state_bit(bool value, QLabel *label, const QString &on, const QString &off, const QString &colour_on, const QString &colour_off) {
    bool valid = this->station->dome()->state_S().is_valid();
    label->setText(valid ? value ? on : off : "?");
    label->setStyleSheet(QString("QLabel { color: %1; }").arg(valid ? value ? colour_on : colour_off : "gray"));
}

/* Display action button text for value `on` in button `button` */
void MainWindow::display_device_button(bool on, QPushButton *button) {
    bool valid = this->station->dome()->state_S().is_valid();
    button->setText(valid ? on ? "turn off" : "turn on" : "no connection");
}

/*  DRY helper function
 *  Display sensor output `value` in `label` with unit `unit` */
void MainWindow::display_sensor_value(float value, QLabel *label, const QString &unit) {
    if (this->station->dome()->state_T().is_valid()) {
        label->setText(QString("%1 %2").arg(value, 4, 'f', 1).arg(unit));
        if (unit == "°C") {
            label->setStyleSheet(QString("QLabel { color: %1; }").arg(Station::temperature_colour(value)));
        } else {
            label->setStyleSheet(QString("QLabel { color: black; }"));
        }
    } else {
        label->setText(QString("? %1").arg(unit));
        label->setStyleSheet(QString("QLabel { color: gray; }"));
    }
}

/*  DRY helper function
 *  Display status of `storage` in QProgressBar `pb` and QLineEdit `le` */
void MainWindow::display_storage_status(const Storage& storage, QProgressBar *pb, QLineEdit *le) {
    QStorageInfo info = storage.info();
    unsigned int total = (int) ((double) info.bytesTotal() / (1 << 30));
    unsigned int used = (int) ((double) info.bytesAvailable() / (1 << 30));
    pb->setMaximum(total);
    pb->setValue(total - used);
    le->setText(storage.get_directory().path());
}

void MainWindow::display_cover_status(void) {
    const DomeStateS &stateS = this->station->dome()->state_S();
    QString text;

    if (stateS.is_valid()) {
        if (stateS.servo_moving()) {
            text = stateS.servo_direction() ? "opening..." : "closing";
        } else {
            text = stateS.cover_safety_position() ? "peeking" :
                 stateS.dome_open_sensor_active() ? "open" :
                 stateS.dome_closed_sensor_active() ? "closed" : "inconsistent!";
        }
    } else {
        text = "invalid";
    }
    this->ui->lb_cover_state->setText(text);

    // Set cover shaft position
    const DomeStateZ &stateZ = this->station->dome()->state_Z();
    this->ui->progress_cover->setEnabled(stateZ.is_valid());
    this->ui->progress_cover->setValue(stateZ.is_valid() ? stateZ.shaft_position() : 0);
}

void MainWindow::display_basic_data(void) {
    const DomeStateS &state = this->station->dome()->state_S();

    if (state.is_valid()) {
        unsigned int time_alive = state.time_alive();
        unsigned int days = time_alive / 86400;
        unsigned int hours = (time_alive % 86400) / 3600;
        unsigned int minutes = (time_alive % 3600) / 60;
        unsigned int seconds = time_alive % 60;
        this->ui->lb_time_alive->setText(QString("%1d %2:%3:%4")
                                         .arg(days)
                                         .arg(hours, 2, 10, QChar('0'))
                                         .arg(minutes, 2, 10, QChar('0'))
                                         .arg(seconds, 2, 10, QChar('0')));

        this->ui->bt_cover_open->setEnabled(!state.dome_open_sensor_active() && this->station->is_manual());
        this->ui->bt_cover_close->setEnabled(!state.dome_closed_sensor_active() && this->station->is_manual());
    } else {
        this->ui->lb_time_alive->setText("???");
        this->ui->bt_cover_open->setEnabled(false);
        this->ui->bt_cover_close->setEnabled(false);
    }

    // Basic byte
    this->display_S_state_bit(state.servo_moving(),                 this->ui->lb_dome_moving,           "moving",  "not moving", "green", "black");
    this->display_S_state_bit(state.servo_direction(),              this->ui->lb_dome_cover_direction,  "opening", "closing", "black", "black");
    this->display_S_state_bit(state.dome_open_sensor_active(),      this->ui->lb_dome_open_sensor,      "active",  "not active", "green", "black");
    this->display_S_state_bit(state.dome_closed_sensor_active(),    this->ui->lb_dome_closed_sensor,    "active",  "not active", "green", "black");

    this->display_S_state_bit(state.lens_heating_active(),          this->ui->lb_lens_heating,          "on", "off", "green", "black");
    this->display_S_state_bit(state.camera_heating_active(),        this->ui->lb_camera_heating,        "on", "off", "green", "black");
    this->display_S_state_bit(state.fan_active(),                   this->ui->lb_fan,                   "on", "off", "green", "black");
    this->display_S_state_bit(state.intensifier_active(),           this->ui->lb_intensifier,           "on", "off", "green", "black");

    this->display_device_button(state.lens_heating_active(),        this->ui->bt_lens_heating);
    this->display_device_button(state.camera_heating_active(),      this->ui->bt_camera_heating);
    this->display_device_button(state.fan_active(),                 this->ui->bt_fan);
    this->display_device_button(state.intensifier_active(),         this->ui->bt_intensifier);

    // Environment byte
    this->display_S_state_bit(state.rain_sensor_active(),           this->ui->lb_rain_sensor,           "raining", "not raining", "blue", "black");
    this->display_S_state_bit(state.light_sensor_active(),          this->ui->lb_light_sensor,          "light", "no light", "light-blue", "black");
    this->display_S_state_bit(state.computer_power_sensor_active(), this->ui->lb_computer_power,        "powered", "not powered", "green", "red");
    this->display_S_state_bit(state.cover_safety_position(),        this->ui->lb_cover_safety_position, "safety", "no", "blue", "black");
    this->display_S_state_bit(state.servo_blocked(),                this->ui->lb_servo_blocked,         "blocked", "no", "blue", "black");

    // Errors byte
    this->display_S_state_bit(state.error_t_lens(),                 this->ui->lb_error_t_lens,          "error",    "ok", "red", "black");
    this->display_S_state_bit(state.error_SHT31(),                  this->ui->lb_error_SHT31,           "error",    "ok", "red", "black");
    this->display_S_state_bit(state.emergency_closing_light(),      this->ui->lb_error_light,           "closed",   "ok", "red", "black");
    this->display_S_state_bit(state.error_watchdog_reset(),         this->ui->lb_error_watchdog_reset,  "reset",    "ok", "red", "black");
    this->display_S_state_bit(state.error_brownout_reset(),         this->ui->lb_error_brownout_reset,  "reset",    "ok", "red", "black");
    this->display_S_state_bit(state.error_computer_power(),         this->ui->lb_error_computer_power,  "error",    "ok", "red", "black");
    this->display_S_state_bit(state.error_t_CPU(),                  this->ui->lb_error_t_CPU,           "error",    "ok", "red", "black");
    this->display_S_state_bit(state.emergency_closing_rain(),       this->ui->lb_error_rain,            "closed",   "ok", "red", "black");
}

void MainWindow::display_env_data(void) {
    const DomeStateT &state = this->station->dome()->state_T();
    if (state.is_valid()) {
        this->ui->group_environment->setTitle(QString("Environment (%1 s)").arg(state.age(), 3, 'f', 1));
    }

    this->display_sensor_value(state.temperature_lens(),    this->ui->lb_t_lens,    "°C");
    this->display_sensor_value(state.temperature_cpu(),     this->ui->lb_t_cpu,     "°C");
    this->display_sensor_value(state.temperature_sht(),     this->ui->lb_t_sht,     "°C");
    this->display_sensor_value(state.humidity_sht(),        this->ui->lb_h_sht,     "%");
}

void MainWindow::display_shaft_position(void) {
    const DomeStateZ &state = this->station->dome()->state_Z();
    this->ui->progress_cover->setValue(state.shaft_position());
}

void MainWindow::display_storage_status(void) {
    this->display_storage_status(this->station->primary_storage(), this->ui->pb_primary, this->ui->le_primary);
    this->display_storage_status(this->station->permanent_storage(), this->ui->pb_permanent, this->ui->le_permanent);
}

void MainWindow::display_station_config(void) {
    this->ui->le_ip->setText(this->station->server()->address().toString());
    this->ui->sb_port->setValue(this->station->server()->port());
    this->ui->le_station_id->setText(this->station->get_id());

    this->ui->dsb_latitude->setValue(this->station->latitude());
    this->ui->dsb_longitude->setValue(this->station->longitude());
    this->ui->dsb_altitude->setValue(this->station->altitude());

    this->ui->dsb_darkness_limit->setValue(this->station->darkness_limit());
    this->ui->dsb_humidity_limit_lower->setValue(this->station->humidity_limit_lower());
    this->ui->dsb_humidity_limit_upper->setValue(this->station->humidity_limit_upper());
}

void MainWindow::display_ufo_state(void) {
    this->ui->le_ufo_path->setText(this->ufo->path());
    this->ui->lb_ufo_state->setText(this->ufo->state_string());
    this->display_storage_status();
}

void MainWindow::display_time(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    ui->label_heartbeat->setText(QString("%1 s").arg((double) this->timer_heartbeat->remainingTime() / 1000, 3, 'f', 1));
}

void MainWindow::display_serial_port_info(void) {
    this->ui->lb_serial_port->setText(this->station->dome()->serial_port_info());
    this->ui->lb_serial_data->setText(this->station->dome()->state_S().is_valid() ? "valid data" : "no data");
}

void MainWindow::display_sun_properties(void) {
    auto hor = this->station->sun_position();
    this->ui->lb_hor_altitude->setText(QString("%1°").arg(hor.theta * Deg, 3, 'f', 3));
    this->ui->lb_hor_altitude->setStyleSheet(QString("QLabel { color: %1; }").arg(Universe::altitude_colour(hor.theta * Deg)));
    this->ui->lb_hor_azimuth->setText(QString("%1°").arg(fmod(hor.phi * Deg + 360.0, 360.0), 3, 'f', 3));

    if (hor.theta * Deg > 0) {
        this->ui->lb_sun_status->setText("day");
        this->ui->lb_sun_status->setToolTip("Sun is above the horizon");
        this->ui->lb_sun_status->setStyleSheet(QString("QLabel { color: %1; }").arg(Universe::altitude_colour(hor.theta * Deg)));
    } else {
        if (this->station->is_dark()) {
            this->ui->lb_sun_status->setText("dark enough");
            this->ui->lb_sun_status->setToolTip("Sun is below the horizon and below the darkness limit");
            this->ui->lb_sun_status->setStyleSheet("QLabel { color: black; }");
        } else {
            this->ui->lb_sun_status->setText("dusk");
            this->ui->lb_sun_status->setToolTip("Sun is below the horizon, but above the darkness limit");
            this->ui->lb_sun_status->setStyleSheet("QLabel { color: blue; }");
        }
    }

    auto equ = Universe::compute_sun_equ();
    this->ui->lb_eq_latitude->setText(QString("%1°").arg(equ[theta] * Deg, 3, 'f', 3));
    this->ui->lb_eq_longitude->setText(QString("%1°").arg(equ[phi] * Deg, 3, 'f', 3));

    auto ecl = Universe::compute_sun_ecl();
    this->ui->lb_ecl_longitude->setText(QString("%1°").arg(ecl[phi] * Deg, 3, 'f', 3));
}
