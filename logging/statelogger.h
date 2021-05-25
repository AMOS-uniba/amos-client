#include <QDateTime>
#include <QTextStream>

#include "logging/baselogger.h"

#ifndef STATELOGGER_H
#define STATELOGGER_H

class StateLogger: public BaseLogger {
    Q_OBJECT
private:
    QString format(const QDateTime & timestamp, const QString & message) const;
public:
    explicit StateLogger(QObject * parent, const QString & filename);

    void log(const QString & message) const;
};

#endif // STATELOGGER_H
