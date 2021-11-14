#ifndef QSERIALPORTMANAGER_H
#define QSERIALPORTMANAGER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QSerialPort>
#include <QWaitCondition>

#include "qserialbuffer.h"
#include "utils/telegram.h"
#include "utils/request.h"


class QSerialPortManager: public QObject {
    Q_OBJECT
private:
    QTimer * m_request_timer;
    QSerialPort * m_port;
    QSerialBuffer * m_buffer;

    QString m_port_name;
    bool m_quit = false;
    QString m_response;
    unsigned int m_robin = 0;

    QDateTime m_last_received;


    static constexpr unsigned int ReadTimeout = 1000;
    static constexpr unsigned int WriteTimeout = 200;
    static constexpr unsigned char Address = 0x99;

    const static Request RequestBasic, RequestEnv, RequestShaft;

    void clear_port(void);

private slots:
    void process_response(void);
    void handle_error(QSerialPort::SerialPortError spe);

public:
    const static SerialPortState SerialPortNotSet, SerialPortOpen, SerialPortError;

    explicit QSerialPortManager(QObject * parent = nullptr);
    ~QSerialPortManager(void);

    void initialize(void);

    void change_settings(const QString & port_name);

    void request_status(void);
    void request(const QByteArray & request);

signals:
    void read_timeout(void);
    void write_timeout(void);

    void port_changed(const QString & port_name);

    void port_state_changed(SerialPortState sps);
    void error(QSerialPort::SerialPortError spe, const QString & message);
    void message_complete(const QByteArray & message);
};

#endif // QSERIALPORTMANAGER_H
