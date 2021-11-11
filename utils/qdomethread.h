#ifndef QDOMETHREAD_H
#define QDOMETHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QSerialPort>
#include <QWaitCondition>

#include "qserialbuffer.h"
#include "utils/telegram.h"

extern EventLogger logger;


class QDomeThread: public QThread {
    Q_OBJECT
private:
    QSerialPort m_port;

    QString m_port_name;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_quit = false;
    unsigned int m_timeout;
    QString m_response;

    QSerialBuffer * m_buffer;


    void run(void) override;
    void request(const QByteArray & request);

    static constexpr unsigned int ReadTimeout = 1000;
    static constexpr unsigned int WriteTimeout = 200;

public:
    explicit QDomeThread(QObject * parent = nullptr);
    ~QDomeThread(void);

    void change_settings(const QString & port_name, const unsigned int timeout, const QString & response);

signals:
    void read_timeout(void);
    void write_timeout(void);

    void error(const QString & message);
    void message_complete(const QByteArray & message);
};

#endif // QDOMETHREAD_H
