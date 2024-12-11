#include <QTimer>
#include <QWaitCondition>

#include "qserialportmanager.h"
#include "logging/eventlogger.h"
#include "utils/telegram.h"

extern EventLogger logger;


const Request QSerialPortManager::RequestBasic        = Request('S', "basic data request");
const Request QSerialPortManager::RequestEnv          = Request('T', "environment data request");
#if OLD_PROTOCOL
const Request QSerialPortManager::RequestShaft        = Request('W', "shaft position request (old protocol)");
#else
const Request QSerialPortManager::RequestShaft        = Request('Z', "shaft position request");
#endif

const SerialPortState QSerialPortManager::Disabled    = SerialPortState('D', "disabled", QColor(160, 160, 160));
const SerialPortState QSerialPortManager::NotSet      = SerialPortState('N', "not set", QColor(96, 96, 96));
const SerialPortState QSerialPortManager::Open        = SerialPortState('O', "connected", QColor(0, 192, 0));
const SerialPortState QSerialPortManager::Error       = SerialPortState('E', "error", QColor(240, 0, 0));


QSerialPortManager::QSerialPortManager(QObject * parent):
    QObject(parent),
    m_enabled(false),
    m_request_timer(nullptr),
    m_port(nullptr)
{
    this->m_port = new QSerialPort(this);
    this->m_port->setBaudRate(QSerialPort::Baud9600);
    this->m_port->setDataBits(QSerialPort::Data8);
    this->m_port->setPortName("");
    this->m_buffer = new QSerialBuffer(this);

    this->connect(this->m_buffer, &QSerialBuffer::message_complete, this, &QSerialPortManager::message_complete);
}

QSerialPortManager::~QSerialPortManager(void) {
    this->m_request_timer->stop();
    delete this->m_request_timer;
    delete this->m_port;
}

void QSerialPortManager::initialize(void) {
    this->m_request_timer = new QTimer(this);
    this->m_request_timer->setInterval(250);
    this->connect(this->m_request_timer, &QTimer::timeout, this, &QSerialPortManager::request_status);
    emit this->log(Concern::SerialPort, Level::Info, "Dome thread initialized");
}

void QSerialPortManager::set_enabled(bool enabled) {
    emit this->log(Concern::SerialPort, Level::Debug,
                   QString("Serial port manager: now %1abled").arg(enabled ? "en" : "dis"));
    this->m_enabled = enabled;
    this->reset();
}

void QSerialPortManager::set_port(const QString & port_name) {
    emit this->log(Concern::SerialPort, Level::Debug,
                   QString("Serial port manager: trying to open port '%1'").arg(port_name));
    this->m_port->close();
    this->m_port->setPortName(port_name);
    this->reset();
}

void QSerialPortManager::reset(void) {
    if (this->is_enabled()) {
        if (this->m_port->open(QIODevice::ReadWrite)) {
            this->connect(this->m_port, &QSerialPort::readyRead, this, &QSerialPortManager::process_response, Qt::UniqueConnection);
            this->connect(this->m_port, &QSerialPort::errorOccurred, this, &QSerialPortManager::handle_error, Qt::UniqueConnection);
            emit this->port_changed(this->m_port->portName());
            emit this->port_state_changed(QSerialPortManager::Open);
            emit this->log(Concern::SerialPort, Level::Info, QString("Opened port '%1'").arg(this->m_port->portName()));
        } else {
            if (this->m_port->portName() == "") {
                emit this->log(Concern::SerialPort, Level::Warning, QString("Cannot open port (empty)"));
                emit this->port_state_changed(QSerialPortManager::NotSet);
            } else {
                emit this->log(Concern::SerialPort, Level::Warning, QString("Cannot open port '%1'").arg(this->m_port->portName()));
                emit this->error(this->m_port->portName(), this->m_port->error(), this->m_port->errorString());
            }
        }
        this->m_request_timer->start();
    } else {
        this->m_port->close();
        this->m_request_timer->stop();
        emit this->port_state_changed(QSerialPortManager::Disabled);
        emit this->log(Concern::SerialPort, Level::Info, QString("Reset '%1' (disabled)").arg(this->m_port->portName()));
    }

}

void QSerialPortManager::request_status(void) {
    static QVector<Request> requests = {RequestBasic, RequestEnv, RequestShaft};

    this->request(requests[this->m_robin].for_telegram());

    this->m_robin = (this->m_robin + 1) % 3;
}

void QSerialPortManager::request(const QByteArray & request) {
    QByteArray encoded = Telegram(QSerialPortManager::Address, request).compose();
    emit this->log(Concern::SerialPort, Level::DebugDetail, QString("Requesting %1 (%2)").arg(request, QString(encoded)));

    if (this->m_port->isOpen()) {
        this->m_port->write(encoded);
    } else {
        if (this->m_port->portName() == "") {
            emit this->port_state_changed(QSerialPortManager::NotSet);
        } else {
            emit this->error(this->m_port->portName(), this->m_port->error(), this->m_port->errorString());
        }
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
    emit this->port_state_changed(QSerialPortManager::Open);
}

void QSerialPortManager::handle_error(QSerialPort::SerialPortError spe) {
    emit this->port_state_changed(QSerialPortManager::Error);
    emit this->error(this->m_port->portName(), spe, this->m_port->errorString());
}
