#include "include.h"

#include "serialthread.h"

extern Log logger;

const Request SerialThread::RequestBasic        = Request('S', "basic data request");
const Request SerialThread::RequestEnv          = Request('T', "environment data request");
const Request SerialThread::RequestShaft        = Request('Z', "shaft position request");

const Command SerialThread::CommandNoOp         = Command('\x00', "no operation");
const Command SerialThread::CommandOpenCover    = Command('\x01', "open cover");
const Command SerialThread::CommandCloseCover   = Command('\x02', "close cover");
const Command SerialThread::CommandFanOn        = Command('\x05', "turn on fan");
const Command SerialThread::CommandFanOff       = Command('\x06', "turn off fan");
const Command SerialThread::CommandIIOn         = Command('\x07', "turn on image intensifier");
const Command SerialThread::CommandIIOff        = Command('\x08', "turn off image intensifier");
const Command SerialThread::CommandHotwireOn    = Command('\x09', "turn on hotwire");
const Command SerialThread::CommandHotwireOff   = Command('\x0A', "turn off hotwire");
const Command SerialThread::CommandResetSlave   = Command('\x0B', "reset slave");


SerialThread::SerialThread(QObject *parent, const QString &port): QThread(parent) {
    this->m_serial.setBaudRate(QSerialPort::Baud9600);
    this->m_serial.setDataBits(QSerialPort::Data8);
    this->m_serial.setParity(QSerialPort::NoParity);
    this->m_serial.setStopBits(QSerialPort::OneStop);
    this->m_serial.setFlowControl(QSerialPort::SoftwareControl);
    this->m_serial.setPortName(port);
}

SerialThread::~SerialThread() {
    this->m_mutex.lock();
    this->m_quit = true;
    this->m_condition.wakeOne();
    this->m_mutex.unlock();
    this->wait();
}

void SerialThread::send_request(const Request &request) {
    logger.info(QString("Sending request '%1'").arg(request.get_display_name()));

    this->m_serial.write(request.for_telegram());
        //logger.info("Waiting for write");
        if (this->m_serial.waitForBytesWritten(this->m_wait_timeout)) {
            //logger.info("Written, waiting for read");
            QByteArray response_data = this->m_serial.readAll();

            if (this->m_serial.waitForReadyRead(this->m_wait_timeout)) {
                response_data = this->m_serial.readAll();
                while (this->m_serial.waitForReadyRead(10)) {
                    response_data += this->m_serial.readAll();
                }
            } else {
                //emit this->timeout(QString("Wait read response timeout %1").arg(QString(request_data)));
            }

            if (response_data.length() > 0) {
                emit this->response(response_data);
            }
        } else {
            emit timeout("Wait write request timeout");
        }
}

void SerialThread::run(void) {
    if (!this->m_serial.open(QIODevice::ReadWrite)) {
        emit error(QString("Cannot open %1, error code %2").arg(this->m_serial.portName()).arg(this->m_serial.error()));
    }

    while (!this->m_quit) {
        if (!this->m_serial.isOpen()) {
            emit error(QString("Serial port %1 is not opened!").arg(this->m_serial.portName()));
        }

        this->send_request(SerialThread::RequestBasic);
        this->msleep(1000);
        this->send_request(SerialThread::RequestEnv);
        this->msleep(1000);
        this->send_request(SerialThread::RequestShaft);
        this->msleep(1000);


        this->m_mutex.lock();
   //     this->m_condition.wait(&this->m_mutex);
        this->m_mutex.unlock();
    }
}
