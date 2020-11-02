#include <QCoreApplication>
#include <QSerialPort>
#include <QTextStream>
#include <QTimer>

class Task: public QObject {
    Q_OBJECT
private:
    QSerialPort *m_serial;
    QTextStream *std;
    QTimer *m_timer;
public:
    Task(QObject *parent = nullptr): QObject(parent) {
        this->m_serial = new QSerialPort();
        this->m_serial->setPortName("COM1");
        this->m_serial->setBaudRate(QSerialPort::Baud9600);
        this->m_serial->setDataBits(QSerialPort::Data8);
        this->m_serial->setParity(QSerialPort::NoParity);
        this->m_serial->setStopBits(QSerialPort::OneStop);

        this->m_timer = new QTimer();
        this->m_timer->setInterval(500);
        this->connect(this->m_timer, &QTimer::timeout, this, &Task::send);
        this->m_timer->start();

        this->std = new QTextStream(stdout);

        if (!this->m_serial->open(QSerialPort::ReadWrite)) {
            *std << "Failed to open port" << Qt::endl;
        } else {
            *std << "Port opened successfully" << Qt::endl;
        }

    //    this->connect(this->m_serial, &QSerialPort::readyRead, this, &Task::process);
    //    this->connect(this->m_serial, &QSerialPort::errorOccurred, this, &Task::error);
    }

    virtual ~Task() {}

    void write(const QByteArray &what) {
        *std << "Written" << Qt::endl;
        this->m_serial->write(what);
    }

public slots:
    void process(void) {
        QByteArray response;
        while (this->m_serial->bytesAvailable()) {
            response += this->m_serial->readAll();
        }
        *this->std << QString("Received '%1'").arg(QString(response)) << Qt::endl;
    }

    void send(void) {
        auto message = QByteArray("U99015390\x0D");
        this->write(message);

        if (this->m_serial->waitForBytesWritten(100)) {
            *std << "Waited for written" << Qt::endl;
            if (this->m_serial->waitForReadyRead(60)) {
                *std << "Waited for read" << Qt::endl;
                QByteArray response = this->m_serial->readAll();
                *std << QString(response) << Qt::endl;
            } else {
                *std << "Wait read response timeout" << Qt::endl;
                QByteArray response = this->m_serial->readAll();
                *std << QString(response) << Qt::endl;
                *std << this->m_serial->errorString() << endl;
            }
        } else {
            *std << "Wait write request timeout" << Qt::endl;
        }
    }

    void error(QSerialPort::SerialPortError error) {
        *std << error << Qt::endl;
    }
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QTextStream std(stdout);

    Task *task = new Task();

        /*std << QString("Writing %1 bytes, written %2").arg(message.length()).arg(written) << Qt::endl;

        if (serial.waitForBytesWritten(100)) {
            std << "Waited for written" << Qt::endl;
            if (serial.waitForReadyRead(100)) {
                std << "Waited for read" << Qt::endl;
                QByteArray response = serial.readAll();
                while (serial.waitForReadyRead(100))
                    response += serial.readAll();

                std << QString(response) << Qt::endl;
            } else {
                std << "Wait read response timeout" << Qt::endl;
            }
        } else {
            std << "Wait write request timeout" << Qt::endl;
        }*/

    return a.exec();
}
