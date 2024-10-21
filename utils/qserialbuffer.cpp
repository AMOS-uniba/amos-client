#include "utils/qserialbuffer.h"


QSerialBuffer::QSerialBuffer(QObject * parent):
    QObject(parent),
    m_data(QByteArray())
{}

void QSerialBuffer::insert(const QByteArray & bytes) {
    for (char b: bytes) {
        this->m_data.append(b);

        if (b == 0x0D) {
            emit this->message_complete(this->m_data);
            this->m_data.clear();
        }
    }
}
