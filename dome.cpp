#include "include.h"

extern Log logger;

/**/

Dome::Dome() {
    this->generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    this->address = 0x99;

    this->refresh_timer = new QTimer(this);
    this->refresh_timer->setInterval(Dome::REFRESH);
    this->connect(this->refresh_timer, &QTimer::timeout, this, &Dome::request_status);
    this->refresh_timer->start();

  //  this->m_serial_port = nullptr;

   /* this->m_buffer = new SerialBuffer();
    this->connect(this->m_buffer, &SerialBuffer::message_complete, this, &Dome::process_message);*/

}

Dome::~Dome() {
  //  delete this->m_serial_port;
    delete this->refresh_timer;
//    delete this->m_buffer;
}

const QMap<CoverState, State> Dome::Cover = {
    {CoverState::CLOSED, {'C', "closed"}},
    {CoverState::OPENING, {'>', "opening"}},
    {CoverState::OPEN, {'O', "open"}},
    {CoverState::CLOSING, {'<', "closing"}},
    {CoverState::SAFETY, {'S', "safety"}},
    {CoverState::UNKNOWN, {'?', "unknown"}},
};

const QMap<TernaryState, State> Dome::Ternary = {
    {TernaryState::OFF, {'0', "off"}},
    {TernaryState::ON, {'1', "on"}},
    {TernaryState::UNKNOWN, {'?', "unknown"}},
};

const QDateTime& Dome::get_last_received(void) const {
    return this->last_received;
}

void Dome::init_serial_port(const QString& port) {
    if (this->m_serial_thread != nullptr) {
        delete this->m_serial_thread;
    }

    this->m_serial_thread = new SerialThread(nullptr, port);

    this->connect(this->m_serial_thread, &SerialThread::error, this, &Dome::process_error);
    this->connect(this->m_serial_thread, &SerialThread::response, this, &Dome::process_message);
    this->connect(this->m_serial_thread, &SerialThread::timeout, this, &Dome::process_timeout);


    this->m_serial_thread->start();
  /*  if (this->m_serial_port != nullptr) {
        delete this->m_serial_port;
    }

    this->m_serial_port = new QSerialPort(this);
    this->m_serial_port->setPortName(port);
    this->m_serial_port->setBaudRate(QSerialPort::Baud9600);
    this->m_serial_port->setDataBits(QSerialPort::Data8);
    this->m_serial_port->open(QSerialPort::ReadWrite);

    this->connect(this->m_serial_port, &QSerialPort::readyRead, this, &Dome::process_response);
    this->connect(this->m_serial_port, &QSerialPort::errorOccurred, this, &Dome::handle_error); */
}

QJsonObject Dome::json(void) const {
    return QJsonObject {
        {"env", QJsonObject {
            {"t_lens", this->state_T.temperature_lens()},
            {"t_cpu", this->state_T.temperature_cpu()},
            {"t_sht", this->state_T.temperature_sht()},
            {"h_sht", this->state_T.humidity_sht()},
        }},
        {"cs", Dome::Cover[this->cover_state].code},
        {"cp", (int) this->state_Z.shaft_position()},
        {"heat", QString(QChar(Ternary[this->heating_state].code))},
        {"ii", QString(QChar(Ternary[this->intensifier_state].code))},
        {"fan", QString(QChar(Ternary[this->fan_state].code))},
    };
}

void Dome::send_command(const Command& command) {
    logger.debug(QString("Sending a command '%1'").arg(command.get_display_name()));
    this->send(command.for_telegram());
}

void Dome::send_request(const Request& request) {
    logger.debug(QString("Sending a request '%1'").arg(request.get_display_name()));
    this->send(request.for_telegram());
}

void Dome::send(const QByteArray& message) {
    logger.debug(QString("Sending message '%1'").arg(QString(message)));
    Telegram telegram(this->address, message);

//    this->m_serial_thread->transaction("COM1", 100, telegram.compose());
}

void Dome::process_timeout(const QString &timeout) {
    logger.error(QString("Timed out: %1").arg(timeout));
}

void Dome::process_error(const QString &error) {
    logger.error(QString("Serial port error: %1").arg(error));
}

void Dome::process_message(const QByteArray &message) {
    try {
        Telegram telegram(message);
        QByteArray decoded = telegram.get_message();
        logger.info(QString("Decoded message %1").arg(QString(decoded.toHex())));

        switch (decoded[0]) {
            case 'S':
                this->state_S = DomeStateS(decoded);
                emit this->state_updated_S();
                break;
            case 'T':
                this->state_T = DomeStateT(decoded);
                emit this->state_updated_T();
                break;
            case 'Z':
                this->state_Z = DomeStateZ(decoded);
                emit this->state_updated_Z();
                break;
            default:
                throw MalformedTelegram(QString("Unknown response type '%1'").arg(decoded[0]));
        }
    } catch (MalformedTelegram &e) {
        logger.error(QString("Malformed message '%1'").arg(QString(message)));
    } catch (InvalidState &e) {
        logger.error(QString("Invalid state: '%1'").arg(e.what()));
    }
}

const DomeStateS& Dome::get_state_S(void) const {
    return this->state_S;
}

const DomeStateT& Dome::get_state_T(void) const {
    return this->state_T;
}

const DomeStateZ& Dome::get_state_Z(void) const {
    return this->state_Z;
}

void Dome::open_cover(void) {
    if (this->cover_state == CoverState::CLOSED) {
        //this->send_command(Dome::CommandOpenCover);
    } else {
        logger.warning("Cover is not closed, ignoring");
    }
}

void Dome::close_cover(void) {
    if (this->cover_state == CoverState::OPEN) {
        //this->send_command(Dome::CommandCloseCover);
    } else {
        logger.warning("Cover is not open, ignoring");
    }
}

void Dome::toggle_lens_heating(void) {
    logger.info("Toggling the lens heating");
    if (this->get_state_S().lens_heating_active()) {
        //this->send_command(Dome::CommandHotwireOn);
    } else {
        //this->send_command(Dome::CommandHotwireOff);
    }
}

void Dome::toggle_fan(void) {
    logger.info("Toggling the fan");
    if (this->get_state_S().fan_active()) {
        //this->send_command(Dome::CommandFanOn);
    } else {
        //this->send_command(Dome::CommandFanOff);
    }
}

void Dome::toggle_intensifier(void) {
    logger.info("Toggling the intensifier");
    if (this->get_state_S().intensifier_active()) {
        //this->send_command(Dome::CommandIIOff);
    } else {
        //this->send_command(Dome::CommandIIOn);
    }
}

void Dome::request_status(void) {
}
