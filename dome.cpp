#include "include.h"

extern Log logger;

const Request Dome::RequestBasic        = Request('S', "basic data request");
const Request Dome::RequestEnv          = Request('T', "environment data request");
const Request Dome::RequestShaft        = Request('Z', "shaft position request");

const Command Dome::CommandNoOp         = Command('\x00', "no operation");
const Command Dome::CommandOpenCover    = Command('\x01', "open cover");
const Command Dome::CommandCloseCover   = Command('\x02', "close cover");
const Command Dome::CommandFanOn        = Command('\x05', "turn on fan");
const Command Dome::CommandFanOff       = Command('\x06', "turn off fan");
const Command Dome::CommandIIOn         = Command('\x07', "turn on image intensifier");
const Command Dome::CommandIIOff        = Command('\x08', "turn off image intensifier");
const Command Dome::CommandHotwireOn    = Command('\x09', "turn on hotwire");
const Command Dome::CommandHotwireOff   = Command('\x0A', "turn off hotwire");
const Command Dome::CommandResetSlave   = Command('\x0B', "reset slave");


Dome::Dome() {
    this->generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    this->address = 0x99;

    this->refresh_timer = new QTimer(this);
    this->refresh_timer->setInterval(Dome::REFRESH);
    this->connect(this->refresh_timer, &QTimer::timeout, this, &Dome::request_status);
    this->refresh_timer->start();

    this->m_serial_port = nullptr;

    this->m_buffer = new SerialBuffer();
    this->connect(this->m_buffer, &SerialBuffer::message_complete, this, &Dome::process_message);
}

Dome::~Dome() {
    delete this->m_serial_port;
    delete this->refresh_timer;
    delete this->m_buffer;
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

void Dome::reset_serial_port(const QString& port) {
    if (this->m_serial_port != nullptr) {
        delete this->m_serial_port;
    }

    this->m_serial_port = new QSerialPort(this);
    this->m_serial_port->setPortName(port);
    this->m_serial_port->setBaudRate(QSerialPort::Baud9600);
    this->m_serial_port->setDataBits(QSerialPort::Data8);
    this->m_serial_port->open(QSerialPort::ReadWrite);

    this->connect(this->m_serial_port, &QSerialPort::readyRead, this, &Dome::process_response);
    this->connect(this->m_serial_port, &QSerialPort::errorOccurred, this, &Dome::handle_error);
}

const QString Dome::serial_port_info(void) const {
    if (this->m_serial_port->isOpen()) {
        return "open";
    } else {
        return QString("Error %1: %2").arg(this->m_serial_port->error()).arg(this->m_serial_port->errorString());
    }
}

QJsonObject Dome::json(void) const {
    return QJsonObject {
        {"s", this->state_S().json()},
        {"t", this->state_T().json()},
        {"z", this->state_Z().json()},
    };
}

void Dome::send_command(const Command& command) const {
    logger.debug(QString("Sending a command '%1'").arg(command.display_name()));
    this->send(command.for_telegram());
}

void Dome::send_request(const Request& request) const {
    logger.debug(QString("Sending a request '%1'").arg(request.display_name()));
    this->send(request.for_telegram());
}

void Dome::send(const QByteArray& message) const {
    Telegram telegram(this->address, message);
    this->m_serial_port->write(telegram.compose());
}

void Dome::process_response(void) {
    QByteArray response;

    while (this->m_serial_port->bytesAvailable()) {
        response += this->m_serial_port->readAll();
    }

    this->m_buffer->insert(response);
}


void Dome::process_message(const QByteArray &message) {
    try {
        Telegram telegram(message);
        QByteArray decoded = telegram.get_message();

        switch (decoded[0]) {
            case 'S':
                this->m_state_S = DomeStateS(decoded);
                emit this->state_updated_S();
                break;
            case 'T':
                this->m_state_T = DomeStateT(decoded);
                emit this->state_updated_T();
                break;
            case 'Z':
                this->m_state_Z = DomeStateZ(decoded);
                emit this->state_updated_Z();
                break;
            case 'C':
                logger.warning("Ignoring C message");
                break;
            default:
                throw MalformedTelegram(QString("Unknown response '%1'").arg(QString(decoded)));
        }
    } catch (MalformedTelegram &e) {
        logger.error(QString("Malformed message '%1'").arg(QString(message)));
    } catch (InvalidState &e) {
        logger.error(QString("Invalid state: '%1'").arg(e.what()));
    }
}

void Dome::handle_error(QSerialPort::SerialPortError error) {
    logger.error(QString("Serial port error %1: %2").arg(error).arg(this->m_serial_port->errorString()));
}

const DomeStateS& Dome::state_S(void) const {
    return this->m_state_S;
}

const DomeStateT& Dome::state_T(void) const {
    return this->m_state_T;
}

const DomeStateZ& Dome::state_Z(void) const {
    return this->m_state_Z;
}


void Dome::open_cover(bool manual) {
    logger.info(QString("Opening the cover (%1)").arg(manual ? "manual" : "automatic"));
    this->send_command(Dome::CommandOpenCover);
}

void Dome::close_cover(bool manual) {
    logger.info(QString("Closing the cover (%1)").arg(manual ? "manual" : "automatic"));
    this->send_command(Dome::CommandCloseCover);
}

void Dome::toggle_lens_heating(void) {
    if (this->state_S().lens_heating_active()) {
        this->send_command(Dome::CommandHotwireOff);
        logger.info("Turned off lens heating");
    } else {
        this->send_command(Dome::CommandHotwireOn);
        logger.info("Turned on lens heating");
    }
}

void Dome::toggle_fan(void) {
    if (this->state_S().fan_active()) {
        this->send_command(Dome::CommandFanOff);
        logger.info("Turned off the fan");
    } else {
        this->send_command(Dome::CommandFanOn);
        logger.info("Turned on the fan");
    }
}

void Dome::toggle_intensifier(void) {
    if (this->state_S().intensifier_active()) {
        this->send_command(Dome::CommandIIOff);
        logger.info("Turned off the image intensifier");
    } else {
        this->send_command(Dome::CommandIIOn);
        logger.info("Turned on the image intensifier");
    }
}

void Dome::request_status(void) {
    switch (this->m_robin) {
        case 0:
            this->send_request(Dome::RequestBasic);
            break;
        case 1:
            this->send_request(Dome::RequestEnv);
            break;
        case 2:
            this->send_request(Dome::RequestShaft);
            break;
    }
    this->m_robin = (this->m_robin + 1) % 3;
}
