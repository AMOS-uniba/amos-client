#include <QTimer>
#include <QWaitCondition>

#include "qserialportmanager.h"
#include "logging/eventlogger.h"
#include "utils/telegram.h"

extern EventLogger logger;


const Request QSerialPortManager::RequestBasic              = Request('S', "basic data request");
const Request QSerialPortManager::RequestEnv                = Request('T', "environment data request");
#if OLD_PROTOCOL
const Request QSerialPortManager::RequestShaft              = Request('W', "shaft position request (old protocol)");
#else
const Request QSerialPortManager::RequestShaft              = Request('Z', "shaft position request");
#endif

const SerialPortState QSerialPortManager::SerialPortNotSet  = SerialPortState('N', "not set", QColor(96, 96, 96));
const SerialPortState QSerialPortManager::SerialPortOpen    = SerialPortState('O', "connected", QColor(0, 192, 0));
const SerialPortState QSerialPortManager::SerialPortError   = SerialPortState('E', "error", QColor(240, 0, 0));


QSerialPortManager::QSerialPortManager(QObject * parent):
    QObject(parent),
    m_request_timer(nullptr),
    m_port(nullptr)
{}

QSerialPortManager::~QSerialPortManager(void) {
    this->m_request_timer->stop();
    delete this->m_request_timer;
    this->m_request_timer = nullptr;
}

void QSerialPortManager::initialize(void) {
    this->m_request_timer = new QTimer(this);
    this->m_request_timer->setInterval(250);
    this->connect(this->m_request_timer, &QTimer::timeout, this, &QSerialPortManager::request_status);

    this->m_buffer = new QSerialBuffer(this);
    emit this->log(Concern::SerialPort, Level::Info, "Dome thread initialized");
    emit this->port_state_changed(QSerialPortManager::SerialPortNotSet);
}

void QSerialPortManager::clear_port(void) {
    delete this->m_port;
    this->m_port = nullptr;
}

void QSerialPortManager::set_port(const QString & port_name) {
    emit this->log(Concern::SerialPort, Level::Debug, QString("Trying to port %1").arg(port_name));
    this->m_port_name = port_name;

    this->clear_port();

    this->m_port = new QSerialPort(this);
    this->m_port->setPortName(port_name);
    this->m_port->setBaudRate(QSerialPort::Baud9600);
    this->m_port->setDataBits(QSerialPort::Data8);

    this->m_request_timer->start();

    this->disconnect(this->m_buffer, &QSerialBuffer::message_complete, this, &QSerialPortManager::message_complete);
    this->connect(this->m_buffer, &QSerialBuffer::message_complete, this, &QSerialPortManager::message_complete);

    if (this->m_port->open(QIODevice::ReadWrite)) {
        this->connect(this->m_port, &QSerialPort::readyRead, this, &QSerialPortManager::process_response);
        this->connect(this->m_port, &QSerialPort::errorOccurred, this, &QSerialPortManager::handle_error);
        emit this->log(Concern::SerialPort, Level::Info, QString("Opened %1").arg(this->m_port->portName()));

        emit this->port_state_changed(QSerialPortManager::SerialPortOpen);
        emit this->port_changed(port_name);
    } else {
        emit this->port_state_changed(QSerialPortManager::SerialPortNotSet);
        emit this->error(this->m_port->error(), this->m_port->errorString());
    }
}

void QSerialPortManager::request_status(void) {
    static QVector<Request> requests = {RequestBasic, RequestEnv, RequestShaft};

    this->request(requests[this->m_robin].for_telegram());

    this->m_robin = (this->m_robin + 1) % 3;
}

void QSerialPortManager::request(const QByteArray & request) {
    QByteArray encoded = Telegram(QSerialPortManager::Address, request).compose();
    emit this->log(Concern::SerialPort, Level::Debug, QString("Requesting %1 (%2)").arg(request, QString(encoded)));

    if (this->m_port->isOpen()) {
        this->m_port->write(encoded);
    } else {
        emit this->port_state_changed(QSerialPortManager::SerialPortNotSet);
    }
}

void QSerialPortManager::process_response(void) {
    emit this->log(Concern::SerialPort, Level::Debug, QString("%1").arg(this->m_last_received.msecsTo(QDateTime::currentDateTimeUtc())));

    this->m_last_received = QDateTime::currentDateTimeUtc();
    this->m_error_counter = 0;

    QByteArray response;
    while (this->m_port->bytesAvailable()) {
        response += this->m_port->readAll();
    }
    this->m_buffer->insert(response);
    emit this->port_state_changed(QSerialPortManager::SerialPortOpen);
}

void QSerialPortManager::handle_error(QSerialPort::SerialPortError spe) {
    emit this->port_state_changed(QSerialPortManager::SerialPortError);
    emit this->error(spe, this->m_port->errorString());
}
