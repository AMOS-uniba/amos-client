#include "domemanager.h"

DomeManager::DomeManager() {
    this->generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

const QString& DomeManager::get_cover_code(void) const {
    return cover_code.find(this->cover_state).value();
}

const QString& DomeManager::get_heating_code(void) const {
    return heating_code.find(this->heating_state).value();
}

const QString& DomeManager::get_intensifier_code(void) const {
    return intensifier_code.find(this->intensifier_state).value();
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

QJsonObject DomeManager::json(void) const {
    QJsonObject message;

    message["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    message["temperature"] = this->temperature;
    message["pressure"] = this->pressure;
    message["humidity"] = this->humidity;
    message["shaft_position"] = QString(this->cover_position);

    message["heating"] = this->get_heating_code();
    message["intensifier"] = this->get_intensifier_code();

    return message;
}

void DomeManager::send_command(const Command& command) const {
    return;
}
