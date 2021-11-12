#ifndef QDOMETHREAD_H
#define QDOMETHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QSerialPort>
#include <QWaitCondition>

#include "qserialbuffer.h"
#include "utils/telegram.h"


class QDomeThread: public QThread {
    Q_OBJECT
private:
    QSerialPort * m_port;

    QString m_port_name;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_quit = false;
    QString m_response;

    QSerialBuffer * m_buffer;

    void run(void) override;

    static constexpr unsigned int ReadTimeout = 1000;
    static constexpr unsigned int WriteTimeout = 200;

    void clear_port(void);

private slots:
    void process_response(void);
    void handle_error(QSerialPort::SerialPortError spe);


public:
    explicit QDomeThread(QObject * parent = nullptr);
    ~QDomeThread(void);

    void change_settings(const QString & port_name);
    void request(const QByteArray & request);

signals:
    void read_timeout(void);
    void write_timeout(void);

    void port_changed(const QString & port_name);

    void error(QSerialPort::SerialPortError spe, const QString & message);
    void message_complete(const QByteArray & message);
};

#endif // QDOMETHREAD_H
