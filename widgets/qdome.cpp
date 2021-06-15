#include <QSerialPort>
#include <QSerialPortInfo>

#include "settings.h"
#include "logging/include.h"

#include "utils/exception.h"
#include "utils/request.h"
#include "utils/telegram.h"

#include "qdome.h"
#include "ui_qdome.h"

extern EventLogger logger;
extern QSettings * settings;

const Request QDome::RequestBasic               = Request('S', "basic data request");
const Request QDome::RequestEnv                 = Request('T', "environment data request");
#ifdef OLD_PROTOCOL
const Request QDome::RequestShaft               = Request('W', "shaft position request (old protocol)");
#else
const Request QDome::RequestShaft               = Request('Z', "shaft position request");
#endif

const Command QDome::CommandNoOp                = Command('\x00', "no operation");
const Command QDome::CommandOpenCover           = Command('\x01', "open cover");
const Command QDome::CommandCloseCover          = Command('\x02', "close cover");
const Command QDome::CommandFanOn               = Command('\x05', "turn on fan");
const Command QDome::CommandFanOff              = Command('\x06', "turn off fan");
const Command QDome::CommandIIOn                = Command('\x07', "turn on image intensifier");
const Command QDome::CommandIIOff               = Command('\x08', "turn off image intensifier");
const Command QDome::CommandHotwireOn           = Command('\x09', "turn on hotwire");
const Command QDome::CommandHotwireOff          = Command('\x0A', "turn off hotwire");
const Command QDome::CommandResetSlave          = Command('\x0B', "reset slave");

const SerialPortState QDome::SerialPortNotSet   = SerialPortState('N', "not set");
const SerialPortState QDome::SerialPortOpen     = SerialPortState('O', "open");
const SerialPortState QDome::SerialPortError    = SerialPortState('E', "error");

const ValueFormatter<double> QDome::TemperatureValueFormatter = [](double value) {
    return QString("%1 °C").arg(value, 3, 'f', 1);
};

const ValueFormatter<double> QDome::HumidityValueFormatter = [](double value) {
    return QString("%1 %").arg(value, 3, 'f', 1);
};

const ColourFormatter<double> QDome::TemperatureColourFormatter = [](double temperature) {
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
};

QDome::QDome(QWidget *parent):
    QGroupBox(parent),
    ui(new Ui::QDome),
    m_station(nullptr),
    m_serial_port(nullptr),
    m_state_S(),
    m_state_T(),
    m_state_Z()
{
    this->m_buffer = new SerialBuffer();

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
    this->connect(this, &QDome::state_updated_S, this, &QDome::display_dome_state);
    this->connect(this, &QDome::state_updated_T, this, &QDome::display_env_data);
    this->connect(this, &QDome::state_updated_Z, this, &QDome::display_shaft_data);

    this->connect(this, &QDome::cover_moved, this->ui->picture, &QDomeWidget::set_cover_position);
    this->connect(this, &QDome::cover_moved, this->ui->pb_cover, &QProgressBar::setValue);
    this->connect(this, &QDome::cover_open, this->ui->picture, &QDomeWidget::set_cover_maximum);
    this->connect(this, &QDome::cover_open, this->ui->pb_cover, &QProgressBar::setMaximum);
    this->connect(this, &QDome::cover_closed, this->ui->picture, &QDomeWidget::set_cover_minimum);
    this->connect(this, &QDome::cover_closed, this->ui->pb_cover, &QProgressBar::setMinimum);

//    this->connect(this->ui->cl_camera_heating, &QControlLine::toggled, this, &QDome::toggle_camera_heating);
    this->connect(this->ui->cl_lens_heating, &QControlLine::toggled, this, &QDome::toggle_hotwire);
    this->connect(this->ui->cl_fan, &QControlLine::toggled, this, &QDome::toggle_fan);
    this->connect(this->ui->cl_ii, &QControlLine::toggled, this, &QDome::toggle_intensifier);

    this->connect(this->ui->co_serial_ports, &QComboBox::currentTextChanged, this, &QDome::set_serial_port);

    this->connect(this->ui->dsb_humidity_limit_lower, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QDome::handle_settings_changed);
    this->connect(this->ui->dsb_humidity_limit_upper, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QDome::handle_settings_changed);
    this->connect(this->ui->bt_apply, &QPushButton::clicked, this, &QDome::apply_settings);
    this->connect(this->ui->bt_discard, &QPushButton::clicked, this, &QDome::discard_settings);
    this->connect(this, &QDome::settings_changed, this, &QDome::discard_settings);

    this->connect(this->m_buffer, &SerialBuffer::message_complete, this, &QDome::process_message);

    this->m_robin_timer = new QTimer(this);
    this->m_robin_timer->setInterval(QDome::Refresh);
    this->connect(this->m_robin_timer, &QTimer::timeout, this, &QDome::request_status);
    this->m_robin_timer->start();

    this->m_serial_watchdog = new QTimer(this);
    this->m_serial_watchdog->setInterval(2000);
    this->connect(this->m_serial_watchdog, &QTimer::timeout, this, &QDome::check_serial_port);
    this->m_serial_watchdog->start();

    emit this->cover_open(400);
    emit this->cover_closed(0);
    emit this->cover_moved(0);
}

QDome::~QDome() {
    delete this->ui;
    delete this->m_robin_timer;
    delete this->m_serial_watchdog;
    delete this->m_buffer;
}

void QDome::initialize(const QStation * const station) {
    this->m_station = station;

    this->check_serial_port();
    this->set_formatters();
    this->load_settings();

    this->display_basic_data(this->m_state_S);
    this->display_env_data(this->m_state_T);
    this->display_shaft_data(this->m_state_Z);
}

void QDome::load_settings(void) {
    this->set_humidity_limits(
        settings->value("dome/humidity_lower", 75.0).toDouble(),
        settings->value("dome/humidity_upper", 80.0).toDouble()
    );
}

/**
 * @brief QDome::set_formatters
 * Resets the formatters for all display lines
 */
void QDome::set_formatters(void) {
    this->ui->fl_time_alive->set_value_formatter(MainWindow::format_duration);

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
    this->ui->fl_t_lens->set_colour_formatter(QDome::TemperatureColourFormatter);
    this->ui->fl_t_CPU->set_value_formatter(QDome::TemperatureValueFormatter);
    this->ui->fl_t_CPU->set_colour_formatter(QDome::TemperatureColourFormatter);
    this->ui->fl_t_SHT31->set_value_formatter(QDome::TemperatureValueFormatter);
    this->ui->fl_t_SHT31->set_colour_formatter(QDome::TemperatureColourFormatter);
    this->ui->fl_h_SHT31->set_value_formatter(QDome::HumidityValueFormatter);
    this->ui->fl_h_SHT31->set_colour_formatter([=](double humidity) -> QColor {
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
    logger.debug(Concern::SerialPort, "Displaying basic data");
    bool valid = state.is_valid();

    this->display_serial_port_info();

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

    this->ui->bt_cover_close->setEnabled(state.is_valid() && this->m_station->is_manual() && !state.dome_closed_sensor_active());
    this->ui->bt_cover_open->setEnabled(state.is_valid() && this->m_station->is_manual() && !state.dome_open_sensor_active());

    this->ui->cl_lens_heating->set_enabled(state.is_valid() && this->m_station->is_manual());
    this->ui->cl_camera_heating->set_enabled(state.is_valid() && this->m_station->is_manual());
    this->ui->cl_fan->set_enabled(state.is_valid() && this->m_station->is_manual());
    this->ui->cl_ii->set_enabled(state.is_valid() && this->m_station->is_manual());
}

void QDome::display_env_data(const DomeStateT & state) {
    logger.debug(Concern::SerialPort, "Displaying env data");

    bool valid = state.is_valid();

    this->display_serial_port_info();

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
    this->display_serial_port_info();

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
    this->ui->lb_cover_comment->setText(this->m_station->state().display_string());
}

const DomeStateS& QDome::state_S(void) const { return this->m_state_S; }
const DomeStateT& QDome::state_T(void) const { return this->m_state_T; }
const DomeStateZ& QDome::state_Z(void) const { return this->m_state_Z; }

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
    logger.debug(Concern::SerialPort, "Displaying serial ports");

    this->ui->co_serial_ports->clear();
    auto serial_ports = QSerialPortInfo::availablePorts();

    for (QSerialPortInfo & sp: serial_ports) {
        this->ui->co_serial_ports->addItem(sp.portName());
    }

    if (serial_ports.length() == 0) {
        this->ui->co_serial_ports->setPlaceholderText("no ports available");
        this->ui->co_serial_ports->setEnabled(false);
    } else {
        this->ui->co_serial_ports->setPlaceholderText("not selected");
        this->ui->co_serial_ports->setEnabled(true);
    }
}

void QDome::clear_serial_port(void) {
    delete this->m_serial_port;
    this->m_serial_port = nullptr;
}

void QDome::set_serial_port(const QString &port) {
    this->clear_serial_port();

    this->m_serial_port = new QSerialPort(this);
    this->m_serial_port->setPortName(port);
    this->m_serial_port->setBaudRate(QSerialPort::Baud9600);
    this->m_serial_port->setDataBits(QSerialPort::Data8);

    if (this->m_serial_port->open(QSerialPort::ReadWrite)) {
        this->connect(this->m_serial_port, &QSerialPort::readyRead, this, &QDome::process_response);
        this->connect(this->m_serial_port, &QSerialPort::errorOccurred, this, &QDome::handle_error);
        logger.info(Concern::SerialPort, QString("Opened %1").arg(this->m_serial_port->portName()));

        emit this->serial_port_changed(port);
    } else {
        logger.error(Concern::SerialPort, QString("Could not open serial port %1: %2")
                     .arg(this->m_serial_port->portName(), this->m_serial_port->errorString()));
    }
}

void QDome::check_serial_port(void) {
    if ((this->m_serial_port == nullptr) || (!this->m_serial_port->isOpen())) {
        logger.debug(Concern::SerialPort, "Serial port not working, resetting");
        this->list_serial_ports();
        this->ui->co_serial_ports->setCurrentIndex(0);
    }
}

const QDateTime& QDome::last_received(void) const {
    return this->m_last_received;
}

SerialPortState QDome::serial_port_state(void) const {
    if (this->m_serial_port == nullptr) {
        return QDome::SerialPortNotSet;
    } else {
        return this->m_serial_port->isOpen() ? QDome::SerialPortOpen : QDome::SerialPortError;
    }
}

void QDome::display_serial_port_info(void) const {
    QString info;
    bool reachable = false;

    if (QSerialPortInfo::availablePorts().length() == 0) {
        info = "no ports available";
    } else {
        if (this->m_serial_port == nullptr) {
            info = "no port selected";
        } else {
            if (this->m_serial_port->isOpen()) {
                if (this->state_S().is_valid()) {
                    info = "valid data";
                    reachable = true;
                } else {
                    info = "no data";
                }
            } else {
                info = QString("error %1: %2").arg(this->m_serial_port->error()).arg(this->m_serial_port->errorString());
            }
        }
    }

    this->ui->lb_serial_port_state->setText(this->serial_port_state().display_string());
    this->ui->lb_serial_data_state->setText(info);
    this->ui->picture->set_reachable(reachable);
}

QJsonObject QDome::json(void) const {
    return QJsonObject {
        {"st", QString(QChar(this->serial_port_state().code()))},
        {"s", this->m_state_S.json()},
        {"t", this->m_state_T.json()},
        {"z", this->m_state_Z.json()},
    };
}

void QDome::send_command(const Command &command) const {
    logger.debug(Concern::SerialPort, QString("Sending a command '%1'").arg(command.display_name()));
    this->send(command.for_telegram());
}

void QDome::send_request(const Request &request) const {
    logger.debug(Concern::SerialPort, QString("Sending a request '%1'").arg(request.display_name()));
    this->send(request.for_telegram());
}

void QDome::send(const QByteArray &message) const {
    if (this->m_serial_port == nullptr) {
        logger.debug_error(Concern::SerialPort, "Cannot send, no serial port set");
        return;
    } else {
        if (!this->m_serial_port->isOpen()) {
            logger.debug_error(Concern::SerialPort, QString("Cannot send, serial port %1 is not open").arg(this->m_serial_port->portName()));
        } else {
            Telegram telegram(this->Address, message);
            this->m_serial_port->write(telegram.compose());
        }
    }
}

void QDome::process_response(void) {
    QByteArray response;
    while (this->m_serial_port->bytesAvailable()) {
        response += this->m_serial_port->readAll();
    }
    this->m_buffer->insert(response);
}

void QDome::process_message(const QByteArray &message) {
    try {
        Telegram telegram(message);
        QByteArray decoded = telegram.get_message();

        switch (decoded[0]) {
            case 'C':
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
                emit this->state_updated_T(this->m_state_T);
                break;
#ifdef OLD_PROTOCOL
            case 'W':
#else
            case 'Z':
#endif
                this->m_state_Z = DomeStateZ(decoded);
                emit this->state_updated_Z(this->m_state_Z);
                break;
            default:
                throw MalformedTelegram(QString("Unknown response '%1'").arg(QString(decoded)));
        }
    } catch (MalformedTelegram &e) {
        logger.error(Concern::SerialPort, QString("Malformed message '%1'").arg(QString(message)));
    } catch (InvalidState &e) {
        logger.error(Concern::SerialPort, QString("Invalid state message: '%1'").arg(e.what()));
    }
}

void QDome::handle_error(QSerialPort::SerialPortError error) {
    logger.error(Concern::SerialPort, QString("Error %1: %2").arg(error).arg(this->m_serial_port->errorString()));
}

void QDome::request_status(void) {
    switch (this->m_robin) {
        case 0:
            this->send_request(QDome::RequestBasic);
            break;
        case 1:
            this->send_request(QDome::RequestEnv);
            break;
        case 2:
            this->send_request(QDome::RequestShaft);
            break;
    }
    this->m_robin = (this->m_robin + 1) % 3;
}

void QDome::toggle_hotwire(void) const {
    if (this->state_S().lens_heating_active()) {
        logger.info(Concern::Operation, "Manual command: turn off the hotwire");
        this->turn_off_hotwire();
    } else {
        logger.info(Concern::Operation, "Manual command: turn on the hotwire");
        this->turn_on_hotwire();
    }
}

void QDome::turn_on_hotwire(void) const { this->send_command(QDome::CommandHotwireOn); }
void QDome::turn_off_hotwire(void) const { this->send_command(QDome::CommandHotwireOff); }

// High level command to open the cover. Opens only if it is dark, or if in override mode.
void QDome::open_cover(void) const {
    if (this->m_station->is_dark() || (this->m_station->is_manual() && this->m_station->is_safety_overridden())) {
        this->send_command(QDome::CommandOpenCover);
    }
}
void QDome::close_cover(void) const { this->send_command(QDome::CommandCloseCover); }

void QDome::toggle_fan(void) const {
    if (this->state_S().fan_active()) {
        logger.info(Concern::Operation, "Manual command: turn off the fan");
        this->turn_off_fan();
    } else {
        logger.info(Concern::Operation, "Manual command: turn on the fan");
        this->turn_on_fan();
    }
}
void QDome::turn_on_fan(void) const { this->send_command(QDome::CommandFanOn); }
void QDome::turn_off_fan(void) const { this->send_command(QDome::CommandFanOff); }

// High level command to turn on the intensifier. Turns on only if it is dark, or if in override mode.
void QDome::toggle_intensifier(void) const {
    if (this->state_S().intensifier_active()) {
        logger.info(Concern::Operation, "Manual command: turn off the image intensifier");
        this->turn_off_intensifier();
    } else {
        logger.info(Concern::Operation, "Manual command: turn on the image intensifier");
        this->turn_on_intensifier();
    }
}

void QDome::turn_on_intensifier(void) const {
    if (this->m_station->is_dark() || (this->m_station->is_manual() && this->m_station->is_safety_overridden())) {
        this->send_command(QDome::CommandIIOn);
    } else {
        logger.warning(Concern::SerialPort, "Command ignored, sun is too high and override is not active");
    }
}
void QDome::turn_off_intensifier(void) const { this->send_command(QDome::CommandIIOff); }

// Humidity limit settings
bool QDome::is_humid(void) const {
    return (this->state_T().humidity_sht() >= this->humidity_limit_lower());
}

bool QDome::is_very_humid(void) const {
    return (this->state_T().humidity_sht() >= this->humidity_limit_upper());
}

double QDome::humidity_limit_lower(void) const { return this->m_humidity_limit_lower; }
double QDome::humidity_limit_upper(void) const { return this->m_humidity_limit_upper; }

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

    settings->setValue("dome/humidity_lower", this->humidity_limit_lower());
    settings->setValue("dome/humidity_upper", this->humidity_limit_upper());

    emit this->settings_changed(new_lower, new_upper);
}

void QDome::handle_settings_changed(void) {
    bool changed = (this->ui->dsb_humidity_limit_lower->value() != this->humidity_limit_lower()) ||
        (this->ui->dsb_humidity_limit_upper->value() != this->humidity_limit_upper());

    this->ui->bt_apply->setText(QString("%1 changes").arg(changed ? "Apply" : "No"));
    this->ui->bt_apply->setEnabled(changed);
    this->ui->bt_discard->setEnabled(changed);
}

void QDome::apply_settings(void) {
    try {
        this->set_humidity_limits(this->ui->dsb_humidity_limit_lower->value(), this->ui->dsb_humidity_limit_upper->value());
    } catch (const ConfigurationError & e) {
        logger.error(Concern::Configuration, e.what());
    }
    this->handle_settings_changed();
}

void QDome::discard_settings(void) {
    this->ui->dsb_humidity_limit_lower->setValue(this->humidity_limit_lower());
    this->ui->dsb_humidity_limit_upper->setValue(this->humidity_limit_upper());
}

void QDome::on_bt_cover_open_clicked() {
    logger.info(Concern::Operation, "Manual command to open the over");
    this->open_cover();
}

void QDome::on_bt_cover_close_clicked() {
    logger.info(Concern::Operation, "Manual command to close the over");
    this->close_cover();
}
