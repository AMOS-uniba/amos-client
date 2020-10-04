#include "include.h"

extern Log logger;

DomeManager::DomeManager() {
    this->generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

const QMap<Command, CommandInfo> DomeManager::Commands = {
    {Command::NOP, {'\x00', "no operation"}},
    {Command::COVER_OPEN, {'\x01', "open cover"}},
    {Command::COVER_CLOSE, {'\x02', "close cover"}},
    {Command::FAN_ON, {'\x05', "turn on fan"}},
    {Command::FAN_OFF, {'\x06', "turn off fan"}},
    {Command::II_ON, {'\x07', "turn on image intensifier"}},
    {Command::II_OFF, {'\x08', "turn off image intensifier"}},
    {Command::SW_RESET, {'\x0b', "software reset"}},
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

char DomeManager::ternary_code(TernaryState state) const {
    return DomeManager::Ternary[state].code;
}

char DomeManager::heating_code(void) const {
    return DomeManager::ternary_code(this->heating_state);
}

char DomeManager::fan_code(void) const {
    return DomeManager::Ternary[this->fan_state].code;
}

char DomeManager::intensifier_code(void) const {
    return DomeManager::ternary_code(this->intensifier_state);
}

QString DomeManager::fan_state_name(void) const {
    return DomeManager::Ternary[this->fan_state].display_name;
}

QString DomeManager::intensifier_state_name(void) const {
    return DomeManager::Ternary[this->intensifier_state].display_name;
}

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

QJsonObject DomeManager::json(void) const {
    QJsonObject message;

    message["temperature"] = this->temperature;
    message["pressure"] = this->pressure;
    message["humidity"] = this->humidity;
    message["cover_position"] = (double) this->cover_position;

    message["heating"] = QString(QChar(this->heating_code()));
    message["intensifier"] = QString(QChar(this->intensifier_code()));
    message["fan"] = QString(QChar(this->fan_code()));

    return message;
}

void DomeManager::send_command(const Command& command) const {
    QString message = QString("C%1").arg(DomeManager::Commands[command].code);
    logger.info(QString("{MOCKUP} Sending a manual command %1").arg(DomeManager::Commands[command].display_name));
    return;
}
