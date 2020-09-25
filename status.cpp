#include "status.h"

QJsonDocument Status::message(void) {
    QJsonObject json;

    json["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    json["dome"] = this->dome_manager.json();
    json["automatic"] = this->automatic;

    return QJsonDocument(json);
}
