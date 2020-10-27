#include "include.h"
#include "serialbuffer.h"

extern Log logger;

SerialBuffer::SerialBuffer(void) {
    this->m_data = QByteArray();
}

void SerialBuffer::insert(const QByteArray &bytes) {
    for (char b: bytes) {
        if (b == 0x0D) {
            this->m_messages.enqueue(this->m_data);
            this->m_data.clear();
            emit this->message_complete();
        } else {
            this->m_data.append(b);
        }
    }

    logger.debug(QString("Buffer now contains '%1' (%2 bytes)").arg(QString(this->m_data)).arg(this->m_data.length()));
}

QByteArray SerialBuffer::pop(void) {
    if (this->m_messages.isEmpty()) {
        return this->m_messages.dequeue();
    } else {
        throw "Buffer empty";
    }
}
