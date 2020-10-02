#include "include.h"

extern Log logger;

DomeManager::DomeManager() {
    this->generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

const QMap<Command, char> DomeManager::CommandCode = {
    {Command::NOP, '\x00'},
    {Command::COVER_OPEN, '\x01'},
    {Command::COVER_CLOSE, '\x02'},
    {Command::FAN_ON, '\x05'},
    {Command::FAN_OFF, '\x06'},
    {Command::II_ON, '\x07'},
    {Command::II_OFF, '\x08'},
    {Command::SW_RESET, '\x0b'}
};

const QMap<Command, QString> DomeManager::CommandName = {
    {Command::NOP, "no operation"},
    {Command::COVER_OPEN, "open cover"},
    {Command::COVER_CLOSE, "close cover"},
    {Command::FAN_ON, "turn on fan"},
    {Command::FAN_OFF, "turn off fan"},
    {Command::II_ON, "turn on image intensifier"},
    {Command::II_OFF, "turn off image intensifier"},
    {Command::SW_RESET, "software reset"}
};

const QMap<CoverState, char> DomeManager::CoverCode = {
    {CoverState::OPEN, 'O'},
    {CoverState::OPENING, '>'},
    {CoverState::CLOSED, 'C'},
    {CoverState::CLOSING, '<'},
    {CoverState::SAFETY, 'S'},
};

const QMap<TernaryState, char> DomeManager::TernaryCode = {
    {TernaryState::OFF, '0'},
    {TernaryState::ON, '1'},
    {TernaryState::UNKNOWN, '?'},
};

const QMap<TernaryState, QString> DomeManager::TernaryName = {
    {TernaryState::OFF, "off"},
    {TernaryState::ON, "on"},
    {TernaryState::UNKNOWN, "unknown"},
};

char DomeManager::cover_code(void) const {
    return DomeManager::CoverCode.find(this->cover_state).value();
}

char DomeManager::ternary_code(TernaryState state) const {
    return DomeManager::TernaryCode.find(state).value();
}

char DomeManager::heating_code(void) const {
    return DomeManager::ternary_code(this->heating_state);
}

char DomeManager::fan_code(void) const {
    return DomeManager::ternary_code(this->fan_state);
}

char DomeManager::intensifier_code(void) const {
    return DomeManager::ternary_code(this->intensifier_state);
}

char DomeManager::command_code(Command command) const {
    return DomeManager::CommandCode.find(command).value();
}

const QString& DomeManager::command_name(Command command) const {
    return DomeManager::CommandName.find(command).value();
}

const QString& DomeManager::ternary_name(TernaryState state) const {
    return DomeManager::TernaryName.find(state).value();
}

const QString& DomeManager::fan_state_name(void) const {
    return this->ternary_name(this->fan_state);
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

    message["heating"] = this->heating_code();
    message["intensifier"] = this->intensifier_code();
    message["fan"] = this->fan_code();

    return message;
}

void DomeManager::send_command(const Command& command) const {
    QString message = QString("C%1").arg(DomeManager::command_code(command));
    logger.info(QString("{MOCKUP} Sending a manual command %1").arg(DomeManager::command_name(command)));
    return;
}
