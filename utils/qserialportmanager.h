#ifndef QSERIALPORTMANAGER_H
#define QSERIALPORTMANAGER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QSerialPort>
#include "qserialbuffer.h"
#include "utils/request.h"
#include "utils/state/serialportstate.h"

#include "logging/eventlogger.h"


class QSerialPortManager: public QObject {
    Q_OBJECT
private:
    bool m_enabled;
    bool m_quit = false;

    QTimer * m_request_timer;
    QSerialPort * m_port;
    QSerialBuffer * m_buffer;

    QString m_response;
    unsigned int m_robin = 0;

    QDateTime m_last_received;
    unsigned int m_error_counter = 0;

    static constexpr unsigned int ReadTimeout = 1000;
    static constexpr unsigned int WriteTimeout = 200;
    static constexpr unsigned char Address = 0x99;

    const static Request RequestBasic, RequestEnv, RequestShaft;

    void clear_port(void);

private slots:
    void process_response(void);
    void handle_error(QSerialPort::SerialPortError spe);
    void reset(void);

public:
    const static SerialPortState Disabled, NotSet, Open, Error;

    explicit QSerialPortManager(QObject * parent = nullptr);
    ~QSerialPortManager(void);

    void initialize(void);

    void request_status(void);
    void request(const QByteArray & request);
    inline bool is_enabled(void) const { return this->m_enabled; };
public slots:
    void set_enabled(bool enabled);
    void set_port(const QString & port_name);

signals:
    void read_timeout(void);
    void write_timeout(void);

    void port_changed(const QString & port_name);
    void port_state_changed(SerialPortState sps);
    void error(const QString & port_name, QSerialPort::SerialPortError spe, const QString & message);
    void message_complete(const QByteArray & message);

    void log(Concern concern, Level level, const QString & message);
};

#endif // QSERIALPORTMANAGER_H
