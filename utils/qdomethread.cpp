#include "qdomethread.h"

extern EventLogger logger;


QDomeThread::QDomeThread(QObject * parent):
    QThread(parent),
    m_port(nullptr)
{
    this->m_buffer = new QSerialBuffer(this);
    logger.info(Concern::SerialPort, "Dome thread created");
}

QDomeThread::~QDomeThread(void) {
    this->m_mutex.lock();
    this->m_quit = true;
    this->m_condition.wakeOne();
    this->m_mutex.unlock();
    this->wait();
}

void QDomeThread::clear_port(void) {
    delete this->m_port;
    this->m_port = nullptr;
}

void QDomeThread::change_settings(const QString & port_name) {
    const QMutexLocker locker(&this->m_mutex);
    logger.info(Concern::SerialPort, QString("Opening port %1").arg(port_name));
    this->m_port_name = port_name;

    this->clear_port();

    this->m_port = new QSerialPort(this);
    this->m_port->setPortName(port_name);
    this->m_port->setBaudRate(QSerialPort::Baud9600);
    this->m_port->setDataBits(QSerialPort::Data8);

    this->disconnect(this->m_buffer, &QSerialBuffer::message_complete, this, &QDomeThread::message_complete);
    this->connect(this->m_buffer, &QSerialBuffer::message_complete, this, &QDomeThread::message_complete);


    if (this->m_port->open(QIODevice::ReadWrite)) {
        this->connect(this->m_port, &QSerialPort::readyRead, this, &QDomeThread::process_response);
        this->connect(this->m_port, &QSerialPort::errorOccurred, this, &QDomeThread::handle_error);
        logger.info(Concern::SerialPort, QString("Opened %1").arg(this->m_port->portName()));

        emit this->port_changed(port_name);
    } else {
        emit this->error(this->m_port->error(), this->m_port->errorString());
    }
}

void QDomeThread::request(const QByteArray & request) {
    logger.info(Concern::SerialPort, QString("Requesting %1").arg(QString(request)));

    if (this->m_port->isOpen()) {
        this->m_port->write(request);
    } else {
        emit this->error(this->m_port->error(), this->m_port->errorString());
    }
}

void QDomeThread::process_response(void) {
    QByteArray response;
    while (this->m_port->bytesAvailable()) {
        response += this->m_port->readAll();
    }
    logger.info(Concern::SerialPort, response);
    this->m_buffer->insert(response);
}

void QDomeThread::handle_error(QSerialPort::SerialPortError spe) {
    emit this->error(spe, this->m_port->errorString());
}


void QDomeThread::run(void) {
    this->exec();

    /*
    bool current_port_name_changed = false;
    QString current_port_name;
    QByteArray current_request = Telegram(0x99, QByteArray("S")).compose();

    this->m_mutex.lock();
    if (current_port_name != this->m_port_name) {
        current_port_name = this->m_port_name;
        current_port_name_changed = true;
    }
    this->m_mutex.unlock();

    logger.info(Concern::SerialPort, "Running");

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
        logger.info(Concern::SerialPort, "Request sent, waiting...");

        this->m_mutex.lock();

        if (current_port_name != this->m_port_name) {
            current_port_name = this->m_port_name;
            current_port_name_changed = true;
        } else {
            current_port_name_changed = false;
        }

        this->m_mutex.unlock();
    }*/
}
