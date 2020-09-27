#include "station.h"

Station::Station() {

}

double Station::get_sun_altitude(void) const {
    auto now = QDateTime::currentDateTimeUtc();

    return 43;
}

double Station::get_sun_azimuth(void) const {
    auto now = QDateTime::currentDateTimeUtc();

    return 180;
}

QJsonObject Station::prepare_heartbeat(void) const {
    QJsonObject json;

    json["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    json["dome"] = this->dome_manager.json();
    json["automatic"] = this->automatic;

    return QJsonObject(json);
}
