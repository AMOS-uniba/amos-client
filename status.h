#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>

#include "domemanager.h"

#ifndef STATUS_H
#define STATUS_H

class Status {
private:
public:
    bool automatic = false;
    DomeManager dome_manager;

    QJsonDocument message(void);
};

#endif // STATUS_H
