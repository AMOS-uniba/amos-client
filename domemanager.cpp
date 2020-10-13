#include "include.h"

extern Log logger;

DomeManager::DomeManager() {
    this->generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    this->address = 0x55;

    this->serial_port = new QSerialPort();
    this->serial_port->open(QSerialPort::ReadWrite);
    this->serial_port->setPortName("COM1");
    this->serial_port->setBaudRate(QSerialPort::Baud9600);
    this->serial_port->setDataBits(QSerialPort::Data8);

    this->connect(this->serial_port, &QSerialPort::readyRead, this, &DomeManager::process_response);
}

const QMap<Command, CommandInfo> DomeManager::Commands = {
    {Command::NOP, {'\x00', "no operation"}},
    {Command::COVER_OPEN, {'\x01', "open cover"}},
    {Command::COVER_CLOSE, {'\x02', "close cover"}},
    // \x03 auxiliary contact on ???
    // \x04 auxiliary contact off ???
    {Command::FAN_ON, {'\x05', "turn on fan"}},
    {Command::FAN_OFF, {'\x06', "turn off fan"}},
    {Command::II_ON, {'\x07', "turn on image intensifier"}},
    {Command::II_OFF, {'\x08', "turn off image intensifier"}},
    {Command::HOTWIRE_ON, {'\x09', "turn on hotwire"}},
    {Command::HOTWIRE_OFF, {'\x0A', "turn off hotwire"}},
    {Command::SW_RESET, {'\x0B', "software reset"}},
};

const QMap<CoverState, State> DomeManager::Cover = {
    {CoverState::CLOSED, {'C', "closed"}},
    {CoverState::OPENING, {'>', "opening"}},
    {CoverState::OPEN, {'O', "open"}},
    {CoverState::CLOSING, {'<', "closing"}},
    {CoverState::SAFETY, {'S', "safety"}},
    {CoverState::UNKNOWN, {'?', "unknown"}},
};

const QMap<TernaryState, State> DomeManager::Ternary = {
    {TernaryState::OFF, {'0', "off"}},
    {TernaryState::ON, {'1', "on"}},
    {TernaryState::UNKNOWN, {'?', "unknown"}},
};

const QDateTime& DomeManager::get_last_received(void) const {
    return this->last_received;
}

void DomeManager::fake_env_data(void) {
    std::uniform_real_distribution<double> td(-20, 30);
    std::normal_distribution<double> pd(100000, 1000);
    std::uniform_real_distribution<double> hd(0, 100);

    this->temperature = td(this->generator);
    this->pressure = pd(this->generator);
    this->humidity = hd(this->generator);

    this->last_received = QDateTime::currentDateTimeUtc();
}

void DomeManager::fake_gizmo_data(void) {
    this->heating_state = TernaryState::UNKNOWN;
    this->intensifier_state = TernaryState::UNKNOWN;
}

void DomeManager::open_cover(void) {
    if (this->cover_state == CoverState::CLOSED) {
        this->send_command(Command::COVER_OPEN);
        this->cover_state = CoverState::OPENING;
    } else {
        logger.warning("Cover is not closed, ignoring");
    }
}

void DomeManager::close_cover(void) {
    if (this->cover_state == CoverState::OPEN) {
        this->send_command(Command::COVER_CLOSE);
        this->cover_state = CoverState::CLOSING;
    } else {
        logger.warning("Cover is not open, ignoring");
    }
}

QJsonObject DomeManager::json(void) const {
    return QJsonObject {
        {"env", QJsonObject {
            {"t", this->temperature},
            {"p", this->pressure},
            {"h", this->humidity},
        }},
        {"cs", DomeManager::Cover[this->cover_state].code},
        {"cp", (int) this->cover_position},
        {"heat", QString(QChar(Ternary[this->heating_state].code))},
        {"ii", QString(QChar(Ternary[this->intensifier_state].code))},
        {"fan", QString(QChar(Ternary[this->fan_state].code))},
    };
}

void DomeManager::send_command(const Command& command) const {
    QByteArray message = QByteArray("C") + QByteArray(1, Commands[command].code);
    logger.info(QString("Sending a command '%1'").arg(DomeManager::Commands[command].display_name));

    Telegram telegram(this->address, message);

    if (this->serial_port->isOpen()) {
        this->serial_port->write(telegram.compose());
    } else {
        logger.error("Could not send command: serial port is not open");
    }
}

void DomeManager::process_response(void) {
    const QByteArray response = this->serial_port->readAll();
    logger.info(QString("Received response: \"%1\"").arg(QString(response)));
    emit this->response_received(response);
}

void DomeManager::toggle_fan(void) {
    logger.info("Toggling the fan");
    if (this->fan_state == TernaryState::OFF) {
        this->send_command(Command::FAN_ON);
    } else {
        this->send_command(Command::FAN_OFF);
    }
}

void DomeManager::toggle_heating(void) {
    logger.info("Toggling the heating");
    if (this->heating_state == TernaryState::OFF) {
        this->send_command(Command::HOTWIRE_ON);
    } else {
        this->send_command(Command::HOTWIRE_OFF);
    }
}

void DomeManager::toggle_intensifier(void) {
    logger.info("Toggling the image intensifier");
    if (this->intensifier_state == TernaryState::OFF) {
        this->send_command(Command::II_ON);
    } else {
        this->send_command(Command::II_OFF);
    }
}

//void DomeManager::response_received(const QByteArray& response) {
//    logger.debug(QString("Response received: '%1'").arg(QString(response)));
//}
