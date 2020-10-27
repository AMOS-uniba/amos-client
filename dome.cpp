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

/*
CommThread::CommThread(QObject *parent): QThread(parent) {}

CommThread::~CommThread() {
    this->mutex.lock();
    this->quit = true;
    this->condition.wakeOne();
    this->mutex.unlock();
    this->wait();
}

void CommThread::transaction(const QString &port_name, int wait_timeout, const QByteArray &request) {
    const QMutexLocker locker(&this->mutex);
    this->port_name = port_name;
    this->wait_timeout = wait_timeout;
    this->request = request;

    if (!this->isRunning()) {
        this->start();
    } else {
        this->condition.wakeOne();
    }
}

void CommThread::run(void) {
    bool changed = false;
    this->mutex.lock();

    QString current_port_name;
    if (current_port_name != this->port_name) {
        current_port_name = this->port_name;
        changed = true;
    }

    int current_wait_timeout = this->wait_timeout;
    QByteArray current_request = this->request;
    this->mutex.unlock();

    QSerialPort serial;

    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::SoftwareControl);
    if (current_port_name.isEmpty()) {
        emit error("No port name specified");
        return;
    }

    while (!this->quit) {
        if (changed) {
            serial.close();
            serial.setPortName(current_port_name);

            if (!serial.open(QIODevice::ReadWrite)) {
                emit error(QString("Cannot open %1, error code %2").arg(this->port_name).arg(serial.error()));
                return;
            } else {
                logger.info(QString("Port %1 opened").arg(current_port_name));
            }
        }
        qDebug() << "Serial error: " << serial.error();
    qDebug() << "Serial errorString: " << serial.errorString();

        const QByteArray request_data = current_request;
        logger.info(QString("Sending '%1' (%2 bytes)").arg(QString(request_data)).arg(request_data.length()));
        serial.write(request_data);
        logger.info("Waiting for write");
        if (serial.waitForBytesWritten(this->wait_timeout)) {
            logger.info("Written, waiting for read");
            if (serial.waitForReadyRead(current_wait_timeout)) {
                QByteArray response_data = serial.readAll();
                while (serial.waitForReadyRead(10)) {
                    response_data += serial.readAll();
                }

                emit this->response(response_data);
            } else {
                emit timeout("Wait read response timeout");
            }
        } else {
            emit timeout("Wait write request timeout");
        }

        this->mutex.lock();
        this->condition.wait(&this->mutex);

        if (current_port_name != this->port_name) {
            current_port_name = this->port_name;
            changed = true;
        } else {
            changed = false;
        }
        current_wait_timeout = this->wait_timeout;
        current_request = this->request;
        this->mutex.unlock();
    }
}
*/

Dome::Dome(): buffer() {
    this->generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    this->address = 0x99;

    this->refresh_timer = new QTimer(this);
    this->refresh_timer->setInterval(Dome::REFRESH);
    this->connect(this->refresh_timer, &QTimer::timeout, this, &Dome::request_status);
    this->refresh_timer->start();

    this->serial_port = nullptr;
}

Dome::~Dome() {
    delete this->serial_port;
    delete this->refresh_timer;
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
    if (this->serial_port != nullptr) {
        delete this->serial_port;
    }

    this->serial_port = new QSerialPort(this);
    this->serial_port->setPortName(port);
    this->serial_port->setBaudRate(QSerialPort::Baud9600);
    this->serial_port->setDataBits(QSerialPort::Data8);
    this->serial_port->open(QSerialPort::ReadWrite);

    this->connect(this->serial_port, &QSerialPort::readyRead, this, &Dome::process_response);
    this->connect(this->serial_port, &QSerialPort::errorOccurred, this, &Dome::handle_error);
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

void Dome::send_command(const Command& command) const {
    logger.debug(QString("Sending a command '%1'").arg(command.get_display_name()));
    this->send(command.for_telegram());
}

void Dome::send_request(const Request& request) const {
    logger.debug(QString("Sending a request '%1'").arg(request.get_display_name()));
    this->send(request.for_telegram());
}

void Dome::send(const QByteArray& message) const {
    logger.info(QString("Sending message '%1'").arg(QString(message)));
    Telegram telegram(this->address, message);
    QByteArray full = telegram.compose();

    this->serial_port->write(full);

  /*  if (this->serial_port->waitForBytesWritten(100)) {
        logger.info("Waited for written");
        if (this->serial_port->waitForReadyRead(100)) {
            logger.info("Waited for read");
            QByteArray response = this->serial_port->readAll();
            while (this->serial_port->waitForReadyRead(100))
                response += this->serial_port->readAll();
            logger.info(QString(response));
        } else {
            logger.error(QString("Wait read response timeout"));
        }
    } else {
        logger.error(QString("Wait write request timeout"));
    }*/
}

void Dome::process_response(void) {
    QByteArray response;

    while (this->serial_port->bytesAvailable()) {
        response += this->serial_port->readAll();
    }

    //logger.debug(QString("Processing response: %1 %2").arg(this->serial_port->bytesAvailable()).arg(QString(response)));
    this->buffer.insert(response);

    logger.debug(QString("Buffer now contains '%1' (%2 bytes)").arg(QString(this->buffer)).arg(this->buffer.length()));

    try {
        Telegram telegram(this->buffer);
        QByteArray message = telegram.get_message();
        logger.info(QString("Received message %1").arg(QString(message.toHex())));

        switch (message[0]) {
            case 'S':
                this->state_S = DomeStateS(message);
                break;
            case 'T':
                this->state_T = DomeStateT(message);
                break;
            case 'Z':
                this->state_Z = DomeStateZ(message);
                break;
            default:
                throw MalformedTelegram(QString("Unknown message type '%1'").arg(message[0]));
        }

        this->buffer.clear();
    } catch (MalformedTelegram& e) {
       // logger.error(QString("Malformed message %1").arg(QString(this->buffer)));
    } catch (InvalidState& e) {
        logger.error(QString("Invalid state encountered: %1").arg(e.what()));
        this->buffer.clear();
    }

}

void Dome::handle_error(QSerialPort::SerialPortError error) {
    logger.error(QString("Serial port error %1: %2").arg(error).arg(this->serial_port->errorString()));
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
        this->send_command(Dome::CommandOpenCover);
    } else {
        logger.warning("Cover is not closed, ignoring");
    }
}

void Dome::close_cover(void) {
    if (this->cover_state == CoverState::OPEN) {
        this->send_command(Dome::CommandCloseCover);
    } else {
        logger.warning("Cover is not open, ignoring");
    }
}

void Dome::toggle_fan(void) {
    logger.info("Toggling the fan");
    if (this->fan_state == TernaryState::OFF) {
        this->send_command(Dome::CommandFanOn);
    } else {
        this->send_command(Dome::CommandFanOff);
    }
}

void Dome::toggle_heating(void) {
    logger.info("Toggling the heating");
    if (this->heating_state == TernaryState::OFF) {
        this->send_command(Dome::CommandHotwireOn);
    } else {
        this->send_command(Dome::CommandHotwireOff);
    }
}

void Dome::toggle_intensifier(void) {
    logger.info("Toggling the image intensifier");
    if (this->intensifier_state == TernaryState::OFF) {
        this->send_command(Dome::CommandIIOn);
    } else {
        this->send_command(Dome::CommandIIOff);
    }
}

void Dome::request_status(void) {
  //  this->send_request(Dome::RequestBasic);
    this->send_request(Dome::RequestEnv);
   // this->send_request(Dome::RequestShaft);
}
