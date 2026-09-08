#include <QSerialPort>
#include <QSerialPortInfo>

#include "logging/include.h"

#include "utils/exceptions.h"
#include "utils/request.h"
#include "utils/telegram.h"
#include "utils/formatters.h"
#include "widgets/qstation.h"

#include "qdome.h"
#include "ui_qdome.h"

extern EventLogger logger;


const Command QDome::CommandNoOp                = Command('\x00', "no operation");
const Command QDome::CommandOpenCover           = Command('\x01', "open cover");
const Command QDome::CommandCloseCover          = Command('\x02', "close cover");
const Command QDome::CommandFanOn               = Command('\x05', "turn on fan");
const Command QDome::CommandFanOff              = Command('\x06', "turn off fan");
const Command QDome::CommandIIOn                = Command('\x07', "turn on image intensifier");
const Command QDome::CommandIIOff               = Command('\x08', "turn off image intensifier");
const Command QDome::CommandHotwireOn           = Command('\x09', "turn on hotwire");
const Command QDome::CommandHotwireOff          = Command('\x0A', "turn off hotwire");
const Command QDome::CommandSoftwareReset       = Command('\x0B', "software reset");

const ValueFormatter<double> QDome::TemperatureValueFormatter = [](double value) {
    return QString("%1 °C").arg(value, 3, 'f', 1);
};

const ValueFormatter<double> QDome::HumidityValueFormatter = [](double value) {
    return QString("%1 %").arg(value, 3, 'f', 1);
};

const QString QDome::DefaultPort = QString("COM1");

QDome::QDome(QWidget * parent):
    QAmosWidget(parent),
    ui(new Ui::QDome),
    m_station(nullptr),
    m_last_received(QDateTime::currentDateTimeUtc()),
    m_thread(nullptr),
    m_spm(nullptr),
    m_sps(QSerialPortManager::NotSet),
    m_state_S(),
    m_state_T(),
    m_state_Z()
{
    this->ui->setupUi(this);

    this->ui->fl_time_alive->set_title("Time alive");

    this->ui->bl_servo_moving->set_title("Servo moving");
    this->ui->bl_servo_direction->set_title("Servo direction");
    this->ui->bl_open_dome_sensor->set_title("Open dome sensor");
    this->ui->bl_closed_dome_sensor->set_title("Closed dome sensor");
    this->ui->bl_safety->set_title("Safety position");
    this->ui->bl_servo_blocked->set_title("Servo blocked");

    this->ui->cl_lens_heating->set_title("Lens heating");
    this->ui->cl_camera_heating->set_title("Camera heating");
    this->ui->cl_fan->set_title("Fan");
    this->ui->cl_ii->set_title("Image intensifier");

    this->ui->fl_t_lens->set_title("Lens temperature");
    this->ui->fl_t_CPU->set_title("CPU temperature");
    this->ui->fl_t_SHT31->set_title("Ambient temperature");
    this->ui->fl_h_SHT31->set_title("Ambient humidity");
    this->ui->bl_rain_sensor->set_title("Rain sensor");
    this->ui->bl_light_sensor->set_title("Light sensor");
    this->ui->bl_master_power->set_title("Master power");

    this->ui->bl_error_t_lens->set_title("Lens temperature");
    this->ui->bl_error_t_SHT31->set_title("Environment temperature");
    this->ui->bl_error_light->set_title("Emergency closing (light)");
    this->ui->bl_error_watchdog->set_title("Watchdog reset");
    this->ui->bl_error_brownout->set_title("Brownout reset");
    this->ui->bl_error_master->set_title("Master power");
    this->ui->bl_error_t_CPU->set_title("CPU temperature");
    this->ui->bl_error_rain->set_title("Emergency closing (rain)");

    this->connect(this, &QDome::state_updated_S, this, &QDome::display_basic_data);
    this->connect(this, &QDome::state_updated_S, this, &QDome::state_updated);
    this->connect(this, &QDome::state_updated_T, this, &QDome::display_env_data);
    this->connect(this, &QDome::state_updated_T, this, &QDome::state_updated);
    this->connect(this, &QDome::state_updated_Z, this, &QDome::display_shaft_data);
    this->connect(this, &QDome::state_updated_Z, this, &QDome::state_updated);
    this->connect(this, &QDome::state_updated, this, &QDome::display_dome_state);

    this->connect(this, &QDome::cover_moved, this->ui->picture, &QDomeWidget::set_cover_position);
    this->connect(this, &QDome::cover_moved, this->ui->pb_cover, &QProgressBar::setValue);
    this->connect(this, &QDome::cover_open, this->ui->picture, &QDomeWidget::set_cover_maximum);
    this->connect(this, &QDome::cover_open, this->ui->pb_cover, &QProgressBar::setMaximum);
    this->connect(this, &QDome::cover_closed, this->ui->picture, &QDomeWidget::set_cover_minimum);
    this->connect(this, &QDome::cover_closed, this->ui->pb_cover, &QProgressBar::setMinimum);

//    this->connect(this->ui->cl_camera_heating, &QControlLine::toggled, this, &QDome::toggle_camera_heating); Why is this disabled?
    this->connect(this->ui->cl_lens_heating, &QControlLine::toggled, this, &QDome::toggle_hotwire);
    this->connect(this->ui->cl_fan, &QControlLine::toggled, this, &QDome::toggle_fan);
    this->connect(this->ui->cl_ii, &QControlLine::toggled, this, &QDome::toggle_intensifier);

    this->m_open_timer = new QTimer(this);
    this->m_open_timer->setInterval(100);
    this->connect(this->m_open_timer, &QTimer::timeout, this, &QDome::set_open_since);
    this->connect(this->m_open_timer, &QTimer::timeout, this, &QDome::set_weather_clear_since);
    this->connect(this->m_open_timer, &QTimer::timeout, this, &QDome::display_data_state);
    this->m_open_timer->start();

    this->m_thread = new QThread(this);
    this->m_spm = new QSerialPortManager();
    this->m_spm->moveToThread(this->m_thread);
    this->connect(this->m_thread, &QThread::started, this->m_spm, &QSerialPortManager::initialize, Qt::QueuedConnection);
    /* The manager must be destroyed by the thread that owns it. Its request timer is created inside
     * initialize(), which runs on the worker thread, so deleting the manager from the main thread --
     * even after quit() and wait() have returned -- kills that timer from the wrong thread and Qt
     * complains twice, once for the explicit stop() and once for ~QObject. It only shows on a live
     * station, because an inactive timer is destroyed without complaint and the timer only runs
     * while the dome is enabled.
     *
     * deleteLater() on QThread::finished is the documented way round it. Note the auto connection:
     * finished is emitted from the worker thread and the manager lives there, so the call is direct,
     * and QThread flushes pending deferred deletes as it winds down. By the time wait() returns the
     * manager is gone -- which is why the destructor must not delete it again, as it once did, on
     * top of a forced-queued version of this same connection. That was the double free.
     *
     * The thread itself is a child of this widget and is deleted with it; it gets no deleteLater,
     * which was the other half of that double free.
     */
    this->connect(this->m_thread, &QThread::finished, this->m_spm, &QObject::deleteLater);
    this->m_thread->start();
}

QDome::~QDome() {
    // wait() returns only once the thread has finished, and finishing is what destroys the manager
    this->m_thread->quit();
    this->m_thread->wait();
    delete this->ui;
}

void QDome::initialize(QSettings * settings) {
    QAmosWidget::initialize(settings);

    this->set_formatters();

    emit this->cover_closed(0);
    emit this->cover_open(400);
    emit this->cover_moved(0);

    this->display_basic_data(this->m_state_S);
    this->display_env_data(this->m_state_T);
    this->display_shaft_data(this->m_state_Z);
    this->display_data_state();

    emit this->ui->cb_enabled->checkStateChanged(this->is_enabled() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    this->ui->co_serial_ports->setCurrentText(settings->value("dome/port").toString());
}

void QDome::connect_slots(void) {
    for (auto && widget: {this->ui->dsb_humidity_limit_lower, this->ui->dsb_humidity_limit_upper}) {
        this->connect(widget, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QDome::settings_changed);
    }
    this->connect(this->ui->cb_require_humidity, &QCheckBox::toggled, this, &QDome::settings_changed);
    this->connect(this->ui->sb_open_settle, QOverload<int>::of(&QSpinBox::valueChanged), this, &QDome::settings_changed);

    this->connect(this, &QDome::serial_port_selected, this->m_spm, &QSerialPortManager::set_port, Qt::QueuedConnection);
    this->connect(this, &QDome::command, this->m_spm, &QSerialPortManager::request, Qt::QueuedConnection);
    this->connect(this->ui->pb_sw_reset, &QPushButton::pressed, this, &QDome::request_sw_reset);
    this->connect(this->ui->cb_enabled, &QCheckBox::checkStateChanged, this, &QDome::set_enabled);
    this->connect(this->ui->cb_enabled, &QCheckBox::checkStateChanged, this->m_spm, &QSerialPortManager::set_enabled, Qt::QueuedConnection);
    this->connect(this->ui->co_serial_ports, &QComboBox::activated, this, [this](int index){
        this->handle_serial_port_selected(this->ui->co_serial_ports->itemText(index));
    });

    this->connect(this->m_spm, &QSerialPortManager::message_complete, this, &QDome::process_message, Qt::QueuedConnection);
    this->connect(this->m_spm, &QSerialPortManager::error, this, &QDome::handle_serial_port_error, Qt::QueuedConnection);
    this->connect(this->m_spm, &QSerialPortManager::port_changed, this, &QDome::handle_serial_port_changed, Qt::QueuedConnection);
    this->connect(this->m_spm, &QSerialPortManager::port_state_changed, this, &QDome::set_serial_port_state, Qt::QueuedConnection);
    this->connect(this->m_spm, &QSerialPortManager::log, this, &QDome::pass_log_message, Qt::QueuedConnection);
}

void QDome::load_defaults(void) {
    this->m_require_humidity = QDome::DefaultRequireHumidity;
    this->set_humidity_limits(QDome::DefaultHumidityLower, QDome::DefaultHumidityUpper);
    this->set_open_settle_time(QDome::DefaultOpenSettle);
    this->handle_serial_port_selected(QDome::DefaultPort);
}

void QDome::load_settings_inner(void) {
    logger.debug(Concern::SerialPort, QString("Loading dome settings"));
    this->set_enabled(
        this->m_settings->value("dome/enabled", QDome::DefaultEnabled).toBool()
    );
    this->load_one("humidity limits",
        [this](void) {
            this->set_humidity_limits(
                this->m_settings->value("dome/humidity_lower", QDome::DefaultHumidityLower).toDouble(),
                this->m_settings->value("dome/humidity_upper", QDome::DefaultHumidityUpper).toDouble()
            );
        },
        [this](void) {
            this->set_humidity_limits(QDome::DefaultHumidityLower, QDome::DefaultHumidityUpper);
        }
    );
    this->m_require_humidity =
        this->m_settings->value("dome/require_humidity", QDome::DefaultRequireHumidity).toBool();
    if (!this->m_require_humidity) {
        logger.warning(Concern::Configuration,
                       "Humidity data is not required: the limits will be applied to whatever the "
                       "last reading was, valid or not");
    }
    this->load_one("settle time before opening",
        [this](void) {
            this->set_open_settle_time(
                this->m_settings->value("dome/open_settle", QDome::DefaultOpenSettle).toInt()
            );
        },
        [this](void) { this->set_open_settle_time(QDome::DefaultOpenSettle); }
    );
    this->handle_serial_port_selected(
        this->m_settings->value("dome/port", QDome::DefaultPort).toString()
    );
}

bool QDome::is_changed(void) const {
    return (
        (this->ui->dsb_humidity_limit_lower->value() != this->humidity_limit_lower()) ||
        (this->ui->dsb_humidity_limit_upper->value() != this->humidity_limit_upper()) ||
        (this->ui->cb_require_humidity->isChecked() != this->requires_humidity()) ||
        (this->ui->sb_open_settle->value() != this->open_settle_time())
    );
}

void QDome::apply_changes_inner(void) {
    if (this->is_changed()) {
        this->set_humidity_limits(this->ui->dsb_humidity_limit_lower->value(), this->ui->dsb_humidity_limit_upper->value());

        const bool require = this->ui->cb_require_humidity->isChecked();
        if (require != this->m_require_humidity) {
            this->m_require_humidity = require;
            this->m_humidity_guard_engaged = false;   // say the reason again under the new setting
            logger.warning(Concern::Configuration, QString("Humidity data is now %1")
                                                       .arg(require ? "required" : "NOT required"));
        }

        if (this->ui->sb_open_settle->value() != this->open_settle_time()) {
            this->set_open_settle_time(this->ui->sb_open_settle->value());
            this->m_settle_wait_logged = false;   // say the reason again under the new setting
        }
    }
    emit this->ui->dsb_humidity_limit_lower->valueChanged(this->humidity_limit_lower());
    emit this->ui->dsb_humidity_limit_upper->valueChanged(this->humidity_limit_upper());
    emit this->ui->sb_open_settle->valueChanged(this->open_settle_time());
}

void QDome::discard_changes_inner(void) {
    this->ui->dsb_humidity_limit_lower->setValue(this->humidity_limit_lower());
    this->ui->dsb_humidity_limit_upper->setValue(this->humidity_limit_upper());
    this->ui->cb_require_humidity->setChecked(this->requires_humidity());
    this->ui->sb_open_settle->setValue(this->open_settle_time());
}

void QDome::save_settings_inner(void) const {
    logger.debug(Concern::SerialPort, QString("Saving settings"));
    this->m_settings->setValue("dome/enabled", this->is_enabled());
    this->m_settings->setValue("dome/humidity_lower", this->humidity_limit_lower());
    this->m_settings->setValue("dome/humidity_upper", this->humidity_limit_upper());
    this->m_settings->setValue("dome/require_humidity", this->requires_humidity());
    this->m_settings->setValue("dome/open_settle", this->open_settle_time());
    this->m_settings->setValue("dome/port", this->ui->co_serial_ports->currentText());
}

void QDome::set_station(const QStation * const station) {
    this->m_station = station;
    this->connect(this, &QDome::state_updated, this->m_station, &QStation::automatic_cover);
}

/**
 * @brief QDome::set_formatters
 * Resets the formatters for all display lines
 */
void QDome::set_formatters(void) {
    this->ui->fl_time_alive->set_value_formatter(Formatters::format_duration);

    this->ui->bl_servo_moving->set_formatters(Qt::darkGreen, Qt::black, "moving", "not moving");
    this->ui->bl_servo_direction->set_formatters(Qt::black, Qt::black, "opening", "closing");
    this->ui->bl_open_dome_sensor->set_formatters(Qt::darkGreen, Qt::black, "open", "not open");
    this->ui->bl_closed_dome_sensor->set_formatters(Qt::darkGreen, Qt::black, "closed", "not closed");
    this->ui->bl_safety->set_formatters(Qt::blue, Qt::black, "safety", "no");
    this->ui->bl_servo_blocked->set_formatters(Qt::red, Qt::black, "blocked", "no");

    this->ui->cl_lens_heating->set_formatters(Qt::darkRed, Qt::black, "on", "off");
    this->ui->cl_camera_heating->set_formatters(Qt::darkRed, Qt::black, "on", "off");
    this->ui->cl_fan->set_formatters(Qt::darkGreen, Qt::black, "on", "off");
    this->ui->cl_ii->set_formatters(Qt::darkGreen, Qt::black, "on", "off");

    this->ui->fl_t_lens->set_value_formatter(QDome::TemperatureValueFormatter);
    this->ui->fl_t_lens->set_colour_formatter(&Formatters::temperature_colour);
    this->ui->fl_t_CPU->set_value_formatter(QDome::TemperatureValueFormatter);
    this->ui->fl_t_CPU->set_colour_formatter(&Formatters::temperature_colour);
    this->ui->fl_t_SHT31->set_value_formatter(QDome::TemperatureValueFormatter);
    this->ui->fl_t_SHT31->set_colour_formatter(&Formatters::temperature_colour);
    this->ui->fl_h_SHT31->set_value_formatter(QDome::HumidityValueFormatter);
    this->ui->fl_h_SHT31->set_colour_formatter([this](double humidity) -> QColor {
        return (humidity < this->humidity_limit_lower()) ? Qt::black :
            (humidity > this->humidity_limit_upper()) ? Qt::red : Qt::blue;
    });

    this->ui->bl_rain_sensor->set_formatters(Qt::blue, Qt::black, "raining", "not raining");
    this->ui->bl_light_sensor->set_formatters(Qt::red, Qt::black, "light", "no light");
    this->ui->bl_master_power->set_formatters(Qt::darkGreen, Qt::red, "powered", "not powered");

    this->ui->bl_error_t_lens->set_formatters(Qt::red, Qt::black, "error", "ok");
    this->ui->bl_error_t_SHT31->set_formatters(Qt::red, Qt::black, "error", "ok");
    this->ui->bl_error_light->set_formatters(Qt::red, Qt::black, "light", "ok");
    this->ui->bl_error_watchdog->set_formatters(Qt::red, Qt::black, "reset", "ok");
    this->ui->bl_error_brownout->set_formatters(Qt::red, Qt::black, "reset", "ok");
    this->ui->bl_error_master->set_formatters(Qt::red, Qt::black, "failed", "ok");
    this->ui->bl_error_t_CPU->set_formatters(Qt::red, Qt::black, "error", "ok");
    this->ui->bl_error_rain->set_formatters(Qt::red, Qt::black, "closed", "ok");
}

void QDome::display_basic_data(const DomeStateS & state) {
    bool valid = state.is_valid();
    this->set_data_state(valid ? "valid data" : "no data");

    this->ui->lb_cover_state->setEnabled(valid);
    this->ui->lb_cover_comment->setEnabled(valid);
    this->ui->pb_cover->setEnabled(valid);
    this->ui->fl_time_alive->set_valid(valid);

    this->ui->bl_servo_moving->set_valid(valid);
    this->ui->bl_servo_direction->set_valid(valid);
    this->ui->bl_open_dome_sensor->set_valid(valid);
    this->ui->bl_closed_dome_sensor->set_valid(valid);
    this->ui->bl_safety->set_valid(valid);
    this->ui->bl_servo_blocked->set_valid(valid);

    this->ui->bl_rain_sensor->set_valid(valid);
    this->ui->bl_light_sensor->set_valid(valid);
    this->ui->bl_master_power->set_valid(valid);

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
    this->ui->fl_time_alive->set_value(state.time_alive());

    this->ui->bl_servo_moving->set_value(state.servo_moving());
    this->ui->bl_servo_direction->set_value(state.servo_direction());
    this->ui->bl_open_dome_sensor->set_value(state.dome_open_sensor_active());
    this->ui->bl_closed_dome_sensor->set_value(state.dome_closed_sensor_active());
    this->ui->bl_safety->set_value(state.cover_safety_position());
    this->ui->bl_servo_blocked->set_value(state.servo_blocked());

    this->ui->bl_rain_sensor->set_value(state.rain_sensor_active());
    this->ui->bl_light_sensor->set_value(state.light_sensor_active());
    this->ui->bl_master_power->set_value(state.computer_power_sensor_active());

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

    bool valid_manual = state.is_valid() && this->m_station->is_manual();
    this->ui->bt_cover_close->setEnabled(valid_manual && !state.dome_closed_sensor_active());
    this->ui->bt_cover_open->setEnabled(valid_manual && !state.dome_open_sensor_active());

    this->ui->cl_lens_heating->set_enabled(valid_manual);
    this->ui->cl_camera_heating->set_enabled(valid_manual);
    this->ui->cl_fan->set_enabled(valid_manual);
    this->ui->cl_ii->set_enabled(valid_manual);
    this->ui->pb_sw_reset->setEnabled(valid_manual);
}

void QDome::display_env_data(const DomeStateT & state) {
    bool valid = state.is_valid();
    this->set_data_state(valid ? "valid data" : "no data");

    this->ui->fl_t_lens->set_valid(valid);
    this->ui->fl_t_CPU->set_valid(valid);
    this->ui->fl_t_SHT31->set_valid(valid);
    this->ui->fl_h_SHT31->set_valid(valid);

    this->ui->fl_t_lens->set_value(state.temperature_lens());
    this->ui->fl_t_CPU->set_value(state.temperature_CPU());
    this->ui->fl_t_SHT31->set_value(state.temperature_sht());
    this->ui->fl_h_SHT31->set_value(state.humidity_sht());
}

void QDome::display_shaft_data(const DomeStateZ & state) {
    this->set_data_state(state.is_valid() ? "valid data" : "no data");

    if (state.is_valid()) {
        emit this->cover_moved(state.shaft_position());
    }
}

void QDome::display_dome_state(void) {
    const DomeStateS & stateS = this->state_S();
    QString text;
    QString colour;

    if (stateS.is_valid()) {
        if (stateS.servo_moving()) {
            text = stateS.servo_direction() ? "opening..." : "closing...";
            colour = stateS.servo_direction() ? "hsv(120, 255, 96)" : "hsv(240, 255, 96)";
        } else {
            text = stateS.cover_safety_position() ? "peeking" :
                 stateS.dome_open_sensor_active() ? "open" :
                 stateS.dome_closed_sensor_active() ? "closed" : "semi-open";
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
    this->ui->lb_cover_comment->setText(this->m_station->state().display_string());
}

QString QDome::status_line(void) const {
    return QString("%1 %2C %3C %4C %5% %6")
        .arg(QString(this->m_state_S.full_text()))
        .arg(this->m_state_T.temperature_sht(), 5, 'f', 1)
        .arg(this->m_state_T.temperature_lens(), 5, 'f', 1)
        .arg(this->m_state_T.temperature_CPU(), 5, 'f', 1)
        .arg(this->m_state_T.humidity_sht(), 5, 'f', 1)
        .arg(this->m_state_Z.shaft_position(), 3);
}

void QDome::list_serial_ports(void) {
    logger.debug(Concern::SerialPort, "Listing serial ports");
    auto old = this->ui->co_serial_ports->currentText();

    this->ui->co_serial_ports->clear();
    auto serial_ports = QSerialPortInfo::availablePorts();

    for (QSerialPortInfo & sp: serial_ports) {
        logger.debug(Concern::SerialPort, sp.portName());
        this->ui->co_serial_ports->addItem(sp.portName());
    }

    if (serial_ports.length() == 0) {
        this->ui->co_serial_ports->setPlaceholderText("no ports available");
        this->ui->co_serial_ports->setEnabled(false);
    } else {
        this->ui->co_serial_ports->setPlaceholderText("not selected");
        this->ui->co_serial_ports->setEnabled(true);
        if (old != "") {
            this->handle_serial_port_changed(old);
        }
    }
}

void QDome::handle_serial_port_selected(const QString & port) {
    logger.debug(Concern::SerialPort, QString("Handling setting of serial port to '%1'").arg(port));
    if (port == "") {
        return;
    } else {
        emit this->serial_port_selected(port);
    }
}

void QDome::handle_serial_port_changed(const QString & port) {
    logger.debug(Concern::SerialPort, QString("Handling change of serial port to '%1'").arg(port));

    const QSignalBlocker blocker(this->ui->co_serial_ports);
    this->ui->co_serial_ports->setCurrentText(port);
    this->save_settings();

    this->m_last_received = QDateTime::currentDateTimeUtc();
    this->set_data_state("no data");
}

/**
 * @brief QDome::set_open_since
 * Remember how long the dome has been open and II active (so that UFO can be opened after a delay)
 */
void QDome::set_open_since(void) {
    const DomeStateS & state = this->state_S();
    if (state.is_valid() && state.dome_open_sensor_active() && state.intensifier_active()) {
        // If there is no valid past state, set it
        if (!this->m_open_since.isValid()) {
            this->m_open_since = QDateTime::currentDateTimeUtc();
        }
        // If there is, everything is fine, so don't touch anything
    } else {
        // Otherwise we are not open, set to invalid
        this->m_open_since = QDateTime();
    }
}

/**
 * @brief QDome::set_weather_clear_since
 * Remember since when the weather has been clear, so that the cover does not open the instant a
 * flapping rain sensor happens to say it is not raining.
 *
 * Only a positive reading of bad weather resets the latch. A state that has aged out neither
 * confirms nor denies anything, and the automatic loop refuses to open on a stale state anyway, so
 * clearing the latch on one dropped telegram would only cost the whole settle time at dusk for no
 * gain in safety.
 */
void QDome::set_weather_clear_since(void) {
    const DomeStateS & state = this->state_S();

    if (!state.is_valid()) {
        return;
    }

    const bool bad_weather = state.rain_sensor_active() || this->is_humid();

    // The first look of the session decides whether the wait is waived, and it is spent either way.
    if (!this->m_weather_assessed) {
        this->m_weather_assessed = true;
        if (!bad_weather) {
            this->m_settle_waived = true;
            logger.info(Concern::Automatic,
                        "Weather is clear on the first reading of this session, so the cover may "
                        "open without waiting the settle time out once");
        } else {
            logger.info(Concern::Automatic,
                        "Weather is not clear on the first reading of this session, so the settle "
                        "time applies before the cover may open");
        }
    }

    if (bad_weather) {
        this->m_weather_clear_since = QDateTime();
        if (this->m_settle_waived) {
            this->m_settle_waived = false;
            logger.info(Concern::Automatic,
                        "Weather has gone bad, the cover must now wait the settle time out");
        }
        return;
    }

    if (!this->m_weather_clear_since.isValid()) {
        this->m_weather_clear_since = QDateTime::currentDateTimeUtc();
    }
}

/**
 * @brief QDome::weather_settled
 * Whether the weather has been clear long enough for the cover to open. Unlike the humidity guard,
 * this one only ever delays an opening: closing is never held up by it.
 */
bool QDome::weather_settled(void) const {
    if (this->m_open_settle_time <= 0) {
        if (this->m_settle_wait_logged) {
            this->m_settle_wait_logged = false;
        }
        return true;
    }

    // Granted by the first weather reading of the session, and spent by the first bad one.
    if (this->m_settle_waived) {
        return true;
    }

    const qint64 clear_for = this->m_weather_clear_since.isValid()
        ? this->m_weather_clear_since.secsTo(QDateTime::currentDateTimeUtc())
        : -1;

    if (clear_for >= this->m_open_settle_time) {
        if (this->m_settle_wait_logged) {
            this->m_settle_wait_logged = false;
            logger.info(Concern::Automatic,
                        QString("Weather has been clear for %1 s, the cover may open").arg(clear_for));
        }
        return true;
    }

    if (!this->m_settle_wait_logged) {
        this->m_settle_wait_logged = true;
        logger.warning(Concern::Automatic,
                       QString("Waiting for the weather to stay clear for %1 s before opening")
                           .arg(this->m_open_settle_time));
    }
    return false;
}

void QDome::set_enabled(int enable) {
    this->list_serial_ports();
    this->m_enabled = (bool) enable;
    logger.info(Concern::Operation, QString("Dome: %2abled").arg(enable ? "en" : "dis"));

    this->ui->inner->setEnabled(enable);
    this->set_data_state(enable ? "no data" : "disabled");
    this->m_last_received = QDateTime::currentDateTimeUtc();

    emit this->enabled_set(enable);
    if (enable) {
        logger.info(Concern::SerialPort, QString("Dome enabled, setting serial port to '%1'")
                                                 .arg(this->ui->co_serial_ports->currentText()));
        emit this->ui->co_serial_ports->activated(this->ui->co_serial_ports->currentIndex());
    }

    this->m_settings->setValue("dome/enabled", this->is_enabled());

    const QSignalBlocker blocker(this->ui->cb_enabled);
    this->ui->cb_enabled->setCheckState(enable ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
}

void QDome::display_data_state(void) const {
    this->ui->lb_serial_data_state->setText(
        QString("%1 (%2)").arg(
            this->data_state(),
            Formatters::format_duration_double(this->last_received().msecsTo(QDateTime::currentDateTimeUtc()) / 1000.0, 1)
        )
    );
    this->ui->picture->set_reachable(this->state_S().is_valid());
}

void QDome::set_serial_port_state(const SerialPortState & state) {
    this->m_sps = state;
    logger.debug(Concern::SerialPort, QString("Port state set to %1").arg(state.display_string()));
    this->ui->lb_serial_port_state->setText(state.display_string());
    this->ui->lb_serial_port_state->setStyleSheet(QString("QLabel { color: %1; }").arg(state.colour().name()));
}

void QDome::set_data_state(const QString & data_state) {
    this->m_data_state = data_state;
}

void QDome::handle_serial_port_error(const QString & port, QSerialPort::SerialPortError error, const QString & message) {
    if (error != QSerialPort::SerialPortError::NoError) {
        logger.error(Concern::SerialPort, QString("Error on port '%1', %2: %3").arg(port).arg(error).arg(message));
        this->set_serial_port_state(QSerialPortManager::Error);
        this->set_data_state(QString("error %1").arg(error));
    }
}

void QDome::pass_log_message(Concern concern, Level level, const QString & message) {
    logger.write(level, concern, message);
}

QJsonObject QDome::json(void) const {
    return QJsonObject {
        {"on", this->is_enabled()},
        {"st", QString(QChar(this->serial_port_state().code()))},
        {"s", this->m_state_S.json()},
        {"t", this->m_state_T.json()},
        {"z", this->m_state_Z.json()},
    };
}

/** Put a command on the wire, unless the very same one has just been sent.
 *
 *  The automatic loop is level-triggered: it reissues a command on every pass for as long as the
 *  state it reads disagrees with the state it wants, and it runs about five times a second -- once
 *  per incoming S/T/Z telegram plus its own one-second timer. A single opening therefore used to
 *  put dozens of identical open commands on the line, turning off the intensifier took one per pass
 *  for as long as the bit stayed set, and the inconsistent-sensors branch, which has no override
 *  escape, floods for as long as a sensor is stuck.
 *
 *  Repeating is not itself wrong and this does not stop it. Nothing acknowledges a command, the
 *  line drops bytes, and one that never arrived has to be issued again -- edge triggering would
 *  trade a flood for a cover that stays shut all night because one telegram was lost. It is the
 *  rate that is wrong. Measured against a stuck closed limit switch, thirty seconds produced 703
 *  identical open commands, some fifty times what the situation calls for, and unbounded for as
 *  long as the sensor stays stuck. At 9600 baud that did not in fact starve the 250 ms state
 *  polling -- the same run answered 114 state requests before and 115 after, so that worry was
 *  unfounded -- but it does drown the log, and it leaves no headroom on a slower or noisier line.
 *
 *  So each distinct command gets its own cooldown. Different commands never share one, so an
 *  emergency close is never held up by an open that preceded it, and the first send of anything is
 *  always immediate. Manual commands from the control buttons go through the same gate: a second
 *  press inside the cooldown asks for something already under way, and the buttons are in any case
 *  disabled once the matching limit sensor reports the cover has arrived.
**/
void QDome::send_command(const Command & command) {
    const QByteArray telegram = command.for_telegram();
    const QDateTime now = QDateTime::currentDateTimeUtc();

    const auto last = this->m_command_sent_at.constFind(telegram);
    if (last != this->m_command_sent_at.constEnd()) {
        const double since = last->msecsTo(now) / 1000.0;
        if (since < QDome::CommandRepeat) {
            logger.detail(Concern::SerialPort,
                          QString("Not repeating the command '%1', sent %2 s ago")
                              .arg(command.display_name()).arg(since, 0, 'f', 1));
            return;
        }
    }

    this->m_command_sent_at[telegram] = now;
    logger.debug(Concern::SerialPort, QString("Sending a command '%1'").arg(command.display_name()));
    emit this->command(telegram);
}

void QDome::process_message(const QByteArray & message) {
    try {
        Telegram telegram(message);
        QByteArray decoded = telegram.get_message();
        this->m_last_received = QDateTime::currentDateTimeUtc();

        switch (decoded[0]) {
            case 'C':
                [[fallthrough]];
            case 'S':
                this->m_state_S = DomeStateS(decoded);
                emit this->state_updated_S(this->m_state_S);

                if (this->m_state_Z.is_valid()) {
                    if (this->m_state_S.dome_open_sensor_active()) {
                        emit this->cover_open(this->m_state_Z.shaft_position());
                    }
                    if (this->m_state_S.dome_closed_sensor_active()) {
                        emit this->cover_closed(this->m_state_Z.shaft_position());
                    }
                }

                break;
            case 'T':
                this->m_state_T = DomeStateT(decoded);
                this->m_humidity_seen = true;
                emit this->state_updated_T(this->m_state_T);
                break;
#if PROTOCOL == 2015
            case 'W':
#elif PROTOCOL == 2020
            case 'Z':
#endif
                this->m_state_Z = DomeStateZ(decoded);
                emit this->state_updated_Z(this->m_state_Z);
                break;
            default:
                throw MalformedTelegram(QString("Unknown response '%1'").arg(QString(decoded)));
        }
        this->set_data_state("valid data");
    /* Nothing may leave this slot. It is invoked from the event loop, and an exception crossing that
     * boundary terminates the process -- which is how a single non-hexadecimal byte on the line used
     * to kill the client: EncodingError comes from Telegram's own decoding, derives from
     * RuntimeException rather than MalformedTelegram, and so matched neither clause here.
     * RuntimeException now covers all three of MalformedTelegram, EncodingError and InvalidState;
     * the two clauses after it exist so that a type nobody anticipated is logged rather than fatal.
     */
    } catch (RuntimeException & e) {
        logger.error(Concern::SerialPort, QString("Bad message '%1': %2").arg(QString(message), e.what()));
        this->set_data_state("invalid data");
    } catch (const std::exception & e) {
        logger.error(Concern::SerialPort, QString("Unexpected failure on message '%1': %2")
                                              .arg(QString(message), e.what()));
        this->set_data_state("invalid data");
    } catch (...) {
        logger.error(Concern::SerialPort, QString("Unknown failure on message '%1'").arg(QString(message)));
        this->set_data_state("invalid data");
    }
}

/** Commands and their wrappers **/

void QDome::toggle_hotwire(void) {
    if (this->state_S().lens_heating_active()) {
        logger.info(Concern::Operation, "Manual command: turn off the hotwire");
        this->turn_off_hotwire();
    } else {
        logger.info(Concern::Operation, "Manual command: turn on the hotwire");
        this->turn_on_hotwire();
    }
}

void QDome::turn_on_hotwire(void) {
    logger.info(Concern::Operation, "Turning on the hotwire");
    this->send_command(QDome::CommandHotwireOn);
}

void QDome::turn_off_hotwire(void) {
    logger.info(Concern::Operation, "Turning off the hotwire");
    this->send_command(QDome::CommandHotwireOff);
}

void QDome::request_sw_reset(void) {
    logger.info(Concern::Operation, "Requesting software reset");
    this->send_command(QDome::CommandSoftwareReset);
}

// High level command to open the cover. Opens only if it is dark, or if in override mode.
void QDome::open_cover(void) {
    if (this->m_station->is_dark_allsky() || (this->m_station->is_manual() && this->m_station->is_safety_overridden())) {
        logger.info(Concern::Operation, "Opening the cover");
        this->send_command(QDome::CommandOpenCover);
    } else {
        logger.warning(Concern::Operation, "Refusing to open the cover: it must be dark, or in manual mode with safety overridden");
    }
}

void QDome::close_cover(void) {
    logger.info(Concern::Operation, "Closing the cover");
    this->send_command(QDome::CommandCloseCover);
}

void QDome::toggle_fan(void) {
    if (this->state_S().fan_active()) {
        logger.info(Concern::Operation, "Manual command: turn off the fan");
        this->turn_off_fan();
    } else {
        logger.info(Concern::Operation, "Manual command: turn on the fan");
        this->turn_on_fan();
    }
}

void QDome::turn_on_fan(void) {
    logger.info(Concern::Operation, "Turning on the fan");
    this->send_command(QDome::CommandFanOn);
}

void QDome::turn_off_fan(void) {
    logger.info(Concern::Operation, "Turning off the fan");
    this->send_command(QDome::CommandFanOff);
}

// High level command to toggle the intensifier. Turns on only if it is dark, or if in override mode.
void QDome::toggle_intensifier(void) {
    if (this->state_S().intensifier_active()) {
        logger.info(Concern::Operation, "Manual command: turn off the image intensifier");
        this->turn_off_intensifier();
    } else {
        logger.info(Concern::Operation, "Manual command: turn on the image intensifier");
        this->turn_on_intensifier();
    }
}

void QDome::turn_on_intensifier(void) {
    if (this->m_station->is_dark_allsky() || (this->m_station->is_manual() && this->m_station->is_safety_overridden())) {
        logger.info(Concern::Operation, "Turning on the image intensifier");
        this->send_command(QDome::CommandIIOn);
    } else {
        logger.error(Concern::Operation, "Refusing to turn on II: it must be dark, or in manual mode with safety overridden");
    }
}

void QDome::turn_off_intensifier(void) {
    logger.info(Concern::Operation, "Turning off the image intensifier");
    this->send_command(QDome::CommandIIOff);
}

/**
 * @brief Whether the humidity reading may be acted upon.
 * Three ways it may not: no usable reading has arrived in this session at all, the last one has aged
 * out of its validity window, or the dome itself reports the SHT31 as failed. The first is treated
 * separately by the callers -- a station whose sensor has never worked keeps observing rather than
 * going dark with nobody watching.
 */
bool QDome::humidity_trustworthy(void) const {
    if (!this->m_humidity_seen) {
        return false;
    }
    if (!this->m_state_T.is_valid()) {
        return false;
    }
    if (this->m_state_S.is_valid() && this->m_state_S.error_SHT31()) {
        return false;
    }
    return true;
}

/**
 * @brief Whether it is too humid to open.
 * Refusing to open on an untrustworthy reading costs nothing but a wait, so this one does not wait
 * out any grace period. A station whose sensor has never produced a reading is exempt: it keeps the
 * behaviour it has always had, and says so in the log.
 */
bool QDome::is_humid(void) const {
    if (this->requires_humidity() && !this->humidity_trustworthy()) {
        if (this->m_humidity_seen) {
            if (!this->m_humidity_guard_engaged) {
                this->m_humidity_guard_engaged = true;
                logger.warning(Concern::Automatic,
                               "Humidity reading is not trustworthy, treating it as too humid to open");
            }
            return true;
        }

        if (!this->m_humidity_guard_engaged) {
            this->m_humidity_guard_engaged = true;
            logger.error(Concern::Automatic,
                         "No humidity reading has ever arrived; observing anyway, but the humidity "
                         "limits are not being enforced. Check the SHT31, or untick \"Require "
                         "humidity data\" to silence this.");
        }
        return (this->state_T().humidity_sht() >= this->humidity_limit_lower());
    }

    if (this->m_humidity_guard_engaged) {
        this->m_humidity_guard_engaged = false;
        logger.info(Concern::Automatic, "Humidity reading is trustworthy again");
    }
    return (this->state_T().humidity_sht() >= this->humidity_limit_lower());
}

/**
 * @brief Whether it is humid enough to close a cover that is already open.
 * Unlike is_humid(), this one waits out HumidityGrace before acting on an untrustworthy reading:
 * closing on a two-second gap in the telegram stream would cost the rest of the night.
 */
bool QDome::is_very_humid(void) const {
    if (this->state_T().humidity_sht() >= this->humidity_limit_upper()) {
        return true;
    }

    if (this->requires_humidity() && this->m_humidity_seen && !this->humidity_trustworthy()) {
        return (this->m_state_T.age() > QDome::HumidityGrace);
    }
    return false;
}

void QDome::set_humidity_limits(const double new_lower, const double new_upper) {
    if ((new_lower < 0) || (new_lower > 100) || (new_upper < 0) || (new_upper > 100) || (new_lower > new_upper)) {
        throw ConfigurationError(QString("Invalid humidity limits: %1% and %2%").arg(new_lower).arg(new_upper));
    }

    this->m_humidity_limit_lower = new_lower;
    this->m_humidity_limit_upper = new_upper;
    logger.info(Concern::Configuration,
                QString("Station's humidity limits set to %1%, %2%")
                    .arg(this->m_humidity_limit_lower)
                    .arg(this->m_humidity_limit_upper)
    );

    emit this->humidity_limits_changed(new_lower, new_upper);
}

void QDome::set_open_settle_time(const int new_settle_time) {
    if ((new_settle_time < 0) || (new_settle_time > QDome::MaxOpenSettle)) {
        throw ConfigurationError(QString("Invalid settle time before opening: %1 s").arg(new_settle_time));
    }

    this->m_open_settle_time = new_settle_time;
    logger.info(Concern::Configuration,
                QString("Weather must be clear for %1 s before the cover opens")
                    .arg(this->m_open_settle_time));
}

void QDome::on_bt_cover_open_clicked() {
    logger.info(Concern::Operation, "Manual command: open the cover");
    this->open_cover();
}

void QDome::on_bt_cover_close_clicked() {
    logger.info(Concern::Operation, "Manual command: close the cover");
    this->close_cover();
}

void QDome::on_dsb_humidity_limit_upper_valueChanged(double value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->dsb_humidity_limit_upper, this->ui->dsb_humidity_limit_upper->value(), this->humidity_limit_upper());
}


void QDome::on_dsb_humidity_limit_lower_valueChanged(double value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->dsb_humidity_limit_lower, this->ui->dsb_humidity_limit_lower->value(), this->humidity_limit_lower());
}

void QDome::on_cb_require_humidity_toggled(bool checked) {
    this->display_changed(this->ui->cb_require_humidity, checked, this->requires_humidity());
}

void QDome::on_sb_open_settle_valueChanged(int value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->sb_open_settle, this->ui->sb_open_settle->value(), this->open_settle_time());
}

