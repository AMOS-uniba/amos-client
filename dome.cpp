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

        const QByteArray request_data = current_request;
        logger.info(QString("Sending '%1'").arg(QString(request_data)));
        serial.write(request_data);
        if (serial.waitForBytesWritten(this->wait_timeout)) {
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


Dome::Dome() {
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

 /*   this->serial_port = new QSerialPort(this);
    this->serial_port->setPortName(port);
    this->serial_port->setBaudRate(QSerialPort::Baud9600);
    this->serial_port->setDataBits(QSerialPort::Data8);
    this->serial_port->open(QSerialPort::ReadWrite);
*/
    //this->connect(this->serial_port, &QSerialPort::readyRead, this, &Dome::process_response);
}

void Dome::fake_env_data(void) {
    std::uniform_real_distribution<double> td(-20, 30);
    std::normal_distribution<double> pd(100000, 1000);
    std::uniform_real_distribution<double> hd(0, 100);

    this->temperature = td(this->generator);
    this->pressure = pd(this->generator);
    this->humidity = hd(this->generator);

    this->last_received = QDateTime::currentDateTimeUtc();
}

void Dome::fake_gizmo_data(void) {
    this->heating_state = TernaryState::UNKNOWN;
    this->intensifier_state = TernaryState::UNKNOWN;
}

void Dome::open_cover(void) {
    if (this->cover_state == CoverState::CLOSED) {
        this->send_command(Dome::CommandOpenCover);
        this->cover_state = CoverState::OPENING;
    } else {
        logger.warning("Cover is not closed, ignoring");
    }
}

void Dome::close_cover(void) {
    if (this->cover_state == CoverState::OPEN) {
        this->send_command(Dome::CommandCloseCover);
        this->cover_state = CoverState::CLOSING;
    } else {
        logger.warning("Cover is not open, ignoring");
    }
}

QJsonObject Dome::json(void) const {
    return QJsonObject {
        {"env", QJsonObject {
            {"t", this->temperature},
            {"p", this->pressure},
            {"h", this->humidity},
        }},
        {"cs", Dome::Cover[this->cover_state].code},
        {"cp", (int) this->cover_position},
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
    Telegram telegram(this->address, message);
    QByteArray full = telegram.compose();

/*
    this->serial_port->write(full);
    if (this->serial_port->waitForBytesWritten(100)) {
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



   /*
    if (this->serial_port->isOpen()) {
        logger.debug("Serial port open");
        logger.debug(QString(full));
        this->serial_port->write(full);
    } else {
        if (this->serial_port->open(QSerialPort::ReadWrite)) {
            logger.info("Opened");
        } else {
            logger.error(QString("Could not open serial port %1: %2").arg(this->serial_port->portName()).arg(this->serial_port->errorString()));
        }
    }

    if (this->serial_port->waitForBytesWritten(100)) {
        logger.info(QString("Bytes written: '%1'").arg(QString(full)));
    } else {
        logger.info("Timeout");
        emit this->write_timeout();
    }

    if (this->serial_port->waitForReadyRead(100)) {
        QByteArray response = this->serial_port->readAll();
        while (this->serial_port->waitForReadyRead(100)) {
            response += this->serial_port->readAll();
        }

        logger.info(QString(response));
        emit this->response_received(response);
    } else {
        logger.info("No response");
    }
        */
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
    //this->send_request(Dome::RequestBasic);
    //this->send_request(Dome::RequestEnv);
    //this->send_request(Dome::RequestShaft);
}