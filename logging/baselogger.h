#include "forward.h"

#ifndef BASELOGGER_H
#define BASELOGGER_H

class BaseLogger: public QObject {
    Q_OBJECT
protected:
    QFile *m_file = nullptr;
public:
    explicit BaseLogger(QObject *parent, const QString &filename);
    ~BaseLogger(void);
};

#endif // BASELOGGER_H
