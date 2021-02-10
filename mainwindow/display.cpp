#include "include.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QPushButton>

extern EventLogger logger;
extern QSettings *settings;

/* Display boolean bit `value` in `label`
   using string `on` with colour `colour_on` if true
   using string `off` with colour `colour_off` if false */
void MainWindow::display_S_state_bit(bool value, QLabel *label, const QString &on, const QString &off, const QString &colour_on, const QString &colour_off) {
    bool valid = this->station->dome()->state_S().is_valid();
    label->setText(valid ? value ? on : off : "?");
    label->setStyleSheet(QString("QLabel { color: %1; }").arg(valid ? value ? colour_on : colour_off : "gray"));
}

/*  Display action button text for value `on` in button `button`.
 *  The button is enabled only if the S state is valid, the station is in manual mode */
void MainWindow::display_device_button(QPushButton *button, bool on) {
    bool valid = this->station->dome()->state_S().is_valid();
    button->setEnabled(valid && this->station->is_manual());
    button->setText(valid ? on ? "turn off" : "turn on" : "no connection");
}

/*  DRY helper function
 *  Display sensor output `value` in `label` with unit `unit` */
void MainWindow::display_sensor_value(QLabel *label, float value, const QString &unit) {
    if (this->station->dome()->state_T().is_valid()) {
        label->setText(QString("%1 %2").arg(value, 4, 'f', 1).arg(unit));
        if (unit == "°C") {
            label->setStyleSheet(QString("QLabel { color: %1; }").arg(Station::temperature_colour(value)));
        } else {
            label->setStyleSheet(QString("QLabel { color: %1; }").arg(
                (value < this->station->humidity_limit_lower()) ? "black" :
                    (value < this->station->humidity_limit_upper()) ? "blue" : "red"
            ));
        }
    } else {
        label->setText(QString("? %1").arg(unit));
        label->setStyleSheet(QString("QLabel { color: gray; }"));
    }
}

void MainWindow::display_cover_status(void) {
    const DomeStateS &stateS = this->station->dome()->state_S();
    QString text;
    QString colour;

    if (stateS.is_valid()) {
        if (stateS.servo_moving()) {
            text = stateS.servo_direction() ? "opening..." : "closing";
            colour = stateS.servo_direction() ? "hsv(120, 255, 96)" : "hsv(240, 255, 96)";
        } else {
            text = stateS.cover_safety_position() ? "peeking" :
                 stateS.dome_open_sensor_active() ? "open" :
                 stateS.dome_closed_sensor_active() ? "closed" : "inconsistent";
            colour = stateS.cover_safety_position() ? "hsv(180, 255, 160)" :
                 stateS.dome_open_sensor_active() ? "hsv(120, 255, 192)" :
                 stateS.dome_closed_sensor_active() ? "black" : "red";
        }
    } else {
        text = "no data";
        colour = "gray";
    }
    this->ui->lb_cover_state->setText(text);
    this->ui->lb_cover_state->setStyleSheet(QString("QLabel {color: %1; }").arg(colour));
    this->ui->lb_cover_comment->setText(this->station->state().display_string());

    // Set cover shaft position
    const DomeStateZ &stateZ = this->station->dome()->state_Z();

    this->ui->progress_cover->setEnabled(stateZ.is_valid());
    this->ui->progress_cover->setValue(stateZ.is_valid() ? stateZ.shaft_position() : 0);
    if (stateS.is_valid() && stateZ.is_valid()) {
        if (stateS.dome_open_sensor_active()) {
            this->ui->progress_cover->setMaximum(stateZ.shaft_position());
        }
    }

}

void MainWindow::display_basic_data(void) {
    const DomeStateS &state = this->station->dome()->state_S();

    if (state.is_valid()) {
        this->ui->lb_time_alive->setText(this->format_duration(state.time_alive()));
    } else {
        this->ui->lb_time_alive->setText("?");
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

    // Environment byte
    this->display_S_state_bit(state.rain_sensor_active(),           this->ui->lb_rain_sensor,           "raining", "not raining", "blue", "black");
    this->display_S_state_bit(state.light_sensor_active(),          this->ui->lb_light_sensor,          "light", "no light", "red", "black");
    this->display_S_state_bit(state.computer_power_sensor_active(), this->ui->lb_computer_power,        "powered", "not powered", "black", "red");
    this->display_S_state_bit(state.cover_safety_position(),        this->ui->lb_cover_safety_position, "safety", "no", "blue", "black");
    this->display_S_state_bit(state.servo_blocked(),                this->ui->lb_servo_blocked,         "blocked", "no", "red", "black");

    // Errors byte
    this->display_S_state_bit(state.error_t_lens(),                 this->ui->lb_error_t_lens,          "error",    "ok", "red", "black");
    this->display_S_state_bit(state.error_SHT31(),                  this->ui->lb_error_SHT31,           "error",    "ok", "red", "black");
    this->display_S_state_bit(state.emergency_closing_light(),      this->ui->lb_error_light,           "closed",   "ok", "red", "black");
    this->display_S_state_bit(state.error_watchdog_reset(),         this->ui->lb_error_watchdog_reset,  "reset",    "ok", "red", "black");
    this->display_S_state_bit(state.error_brownout_reset(),         this->ui->lb_error_brownout_reset,  "reset",    "ok", "red", "black");
    this->display_S_state_bit(state.error_computer_power(),         this->ui->lb_error_computer_power,  "error",    "ok", "red", "black");
    this->display_S_state_bit(state.error_t_CPU(),                  this->ui->lb_error_t_CPU,           "error",    "ok", "red", "black");
    this->display_S_state_bit(state.emergency_closing_rain(),       this->ui->lb_error_rain,            "closed",   "ok", "red", "black");

    // Also display the correct labels on device control buttons
    this->display_device_buttons_state();
}

void MainWindow::display_device_buttons_state(void) {
    const DomeStateS &state = this->station->dome()->state_S();
    this->display_device_button(this->ui->bt_lens_heating,          state.lens_heating_active());
    this->display_device_button(this->ui->bt_camera_heating,        state.camera_heating_active());
    this->display_device_button(this->ui->bt_fan,                   state.fan_active());
    this->display_device_button(this->ui->bt_intensifier,           state.intensifier_active());

    this->ui->bt_cover_close->setEnabled(state.is_valid() && this->station->is_manual() && !this->station->dome()->state_S().dome_closed_sensor_active());
    this->ui->bt_cover_open->setEnabled(state.is_valid() && this->station->is_manual() && !this->station->dome()->state_S().dome_open_sensor_active());
}

void MainWindow::display_env_data(void) {
    const DomeStateT &state = this->station->dome()->state_T();
    if (state.is_valid()) {
        this->ui->group_environment->setTitle(QString("Environment (%1 s)").arg(state.age(), 3, 'f', 1));
    }

    this->display_sensor_value(this->ui->lb_t_lens, state.temperature_lens(),   "°C");
    this->display_sensor_value(this->ui->lb_t_cpu,  state.temperature_cpu(),    "°C");
    this->display_sensor_value(this->ui->lb_t_sht,  state.temperature_sht(),    "°C");
    this->display_sensor_value(this->ui->lb_h_sht,  state.humidity_sht(),       "%");
}

void MainWindow::display_shaft_position(void) {
    const DomeStateZ &state = this->station->dome()->state_Z();
    this->ui->progress_cover->setValue(state.shaft_position());
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

void MainWindow::display_ufo_settings(void) {
    this->ui->cb_ufo_auto->setChecked(this->station->ufo_manager()->autostart());
    this->ui->le_ufo_path->setText(this->station->ufo_manager()->path());
}

void MainWindow::display_ufo_state(void) {
    this->ui->lb_ufo_state->setText(this->station->ufo_manager()->state_string());
}

void MainWindow::display_time(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    ui->lb_next_heartbeat->setText(QString("%1 s").arg((double) this->timer_heartbeat->remainingTime() / 1000, 3, 'f', 1));
    this->ui->lb_uptime->setText(
        this->format_duration(settings->value("run/last_run").toDateTime().secsTo(QDateTime::currentDateTimeUtc()))
    );
}

void MainWindow::display_serial_port_info(void) {
    this->ui->lb_serial_port->setText(this->station->dome()->serial_port_state().display_string());
    this->ui->lb_serial_data->setText(this->station->dome()->state_S().is_valid() ? "valid data" : "no data");
}

void MainWindow::display_window_title(void) {
#ifdef OLD_PROTOCOL
    this->setWindowTitle(QString("AMOS client (old protocol) [%1 mode]%2")
                         .arg(this->station->is_manual() ? "manual" : "automatic")
                         .arg(this->station->is_safety_overridden() ? " [safety override]" : "")
    );
#else
    this->setWindowTitle(QString("AMOS client [%1 mode]%2")
                         .arg(this->station->is_manual() ? "manual" : "automatic")
                         .arg(this->station->is_safety_overridden() ? " [safety override]" : "")
    );
#endif
}

void MainWindow::display_serial_ports(void) {
    logger.debug(Concern::SerialPort, "Displaying serial ports");
    this->ui->co_serial_ports->clear();
    serial_ports = QSerialPortInfo::availablePorts();
    for (QSerialPortInfo sp: serial_ports) {
        this->ui->co_serial_ports->addItem(sp.portName());
    }
}
