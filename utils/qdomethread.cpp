#include "qdomethread.h"

QDomeThread::QDomeThread(QObject * parent):
    QThread(parent)
{
    this->m_buffer = new QSerialBuffer();
}

QDomeThread::~QDomeThread(void) {
    this->m_mutex.lock();
    this->m_quit = true;
    this->m_condition.wakeOne();
    this->m_mutex.unlock();
    this->wait();
}

void QDomeThread::change_settings(const QString & port_name, const unsigned int timeout, const QString & response) {
    const QMutexLocker locker(&this->m_mutex);
    this->m_port_name = port_name;
    this->m_timeout = timeout;
    this->m_response = response;

    if (!this->isRunning()) {
        this->start();
    } else {
        this->m_condition.wakeOne();
    }

    this->connect(this->m_buffer, &QSerialBuffer::message_complete, this, &QDomeThread::message_complete);
}

void QDomeThread::request(const QByteArray & request) {
    this->m_port.write(request);

    if (this->m_port.waitForBytesWritten(this->m_timeout)) {
        if (this->m_port.waitForReadyRead(QDomeThread::ReadTimeout)) {
            this->m_buffer->insert(this->m_port.readAll());

            while (this->m_port.waitForReadyRead(10)) {
                this->m_buffer->insert(this->m_port.readAll());
            }
        } else {
            emit this->read_timeout();
        }
    } else {
        emit this->write_timeout();
    }
}

void QDomeThread::run(void) {
    bool current_port_name_changed = false;
    QString current_port_name;
    QByteArray current_request = Telegram(0x99, QByteArray("S")).compose();

    this->m_mutex.lock();
    if (current_port_name != this->m_port_name) {
        current_port_name = this->m_port_name;
        current_port_name_changed = true;
    }
    this->m_mutex.unlock();

    while (!this->m_quit) {
        logger.info(Concern::SerialPort, "In loop");
        if (current_port_name_changed) {
            this->m_port.close();
            this->m_port.setPortName(current_port_name);

            if (!this->m_port.open(QIODevice::ReadWrite)) {
                emit this->error(QString("Cannot open %1, error code %2").arg(this->m_port_name, this->m_port.error()));
            }
        }

        this->request(current_request);

        this->m_mutex.lock();
        this->m_condition.wait(&this->m_mutex);

        if (current_port_name != this->m_port_name) {
            current_port_name = this->m_port_name;
            current_port_name_changed = true;
        } else {
            current_port_name_changed = false;
        }

        this->m_mutex.unlock();
    }
}
