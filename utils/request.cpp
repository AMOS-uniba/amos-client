#include "include.h"

extern EventLogger logger;

Request::Request(unsigned char code, const QString &display_name):
    m_code(code),
    m_display_name(display_name) {}


QByteArray Request::for_telegram(void) const {
    return QByteArray(1, this->m_code);
}

Command::Command(unsigned char subcode, const QString &display_name):
    Request('C', display_name),
    m_subcode(subcode) {}

QByteArray Command::for_telegram(void) const {
    return QByteArray(1, this->m_code) + QByteArray(1, this->m_subcode);
}

const QString& Request::display_name(void) const {
    return this->m_display_name;
}
