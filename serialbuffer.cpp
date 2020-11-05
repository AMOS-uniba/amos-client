#include "include.h"
#include "serialbuffer.h"

extern Log logger;

SerialBuffer::SerialBuffer(void) {
    this->m_data = QByteArray();
}

void SerialBuffer::insert(const QByteArray &bytes) {
    for (char b: bytes) {
        this->m_data.append(b);

        if (b == 0x0D) {
            emit this->message_complete(this->m_data);
            this->m_data.clear();
        }
    }
}
