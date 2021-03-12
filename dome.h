#include <random>

#include <QSerialPort>
#include <QTimer>
#include <QJsonObject>

#ifndef DOMEMANAGER_H
#define DOMEMANAGER_H

#include "forward.h"

class Dome: public QObject {
    Q_OBJECT
private:
    constexpr static unsigned int REFRESH = 300;

    unsigned char m_address;
    QDateTime m_last_received;
public:
    Dome();
    ~Dome();

public slots:
signals:
    void read_timeout(void) const;
    void write_timeout(void) const;
    void response_received(const QByteArray &response) const;

    void serial_port_changed(const QString &name) const;
};

#endif // DOMEMANAGER_H
