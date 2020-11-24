#include "forward.h"

#include "logging/baselogger.h"

#ifndef STATLOG_H
#define STATLOG_H

class StateLogger: public BaseLogger {
    Q_OBJECT
private:
    QString format(const QDateTime &timestamp, const QString &message) const;
public:
    explicit StateLogger(QObject *parent, const QString &filename);
    ~StateLogger(void);

    void log(const QString &message) const;
};


#endif // STATLOG_H
