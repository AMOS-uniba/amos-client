#ifndef SERIALTHREAD_H
#define SERIALTHREAD_H

#include <QThread>
#include <QSerialPort>

#include "forward.h"

class SerialThread: public QThread {
    Q_OBJECT
private:
    const static Request RequestBasic, RequestEnv, RequestShaft;
    const static Command CommandNoOp;
    const static Command CommandOpenCover, CommandCloseCover;
    const static Command CommandFanOn, CommandFanOff;
    const static Command CommandIIOn, CommandIIOff;
    const static Command CommandHotwireOn, CommandHotwireOff;
    const static Command CommandResetSlave;

    QString m_port_name;
    QByteArray m_request;
    int m_wait_timeout = 0;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_quit = false;

    QSerialPort m_serial;

    void run(void) override;
public:
    explicit SerialThread(QObject *parent = nullptr, const QString &port = "COM1");
    ~SerialThread(void);

    void send_request(const Request &request);
signals:
    void response(const QByteArray &message);
    void error(const QString &message);
    void timeout(const QString &message);
};

#endif // SERIALTHREAD_H
