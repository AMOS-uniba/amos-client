#include "include.h"

Request::Request(unsigned char _code, const QString& _display_name):
    code(_code),
    display_name(_display_name) {}


QByteArray Request::for_telegram(void) const {
    return QByteArray(1, this->code);
}


Command::Command(unsigned char _subcode, const QString& _display_name):
    Request('C', _display_name),
    subcode(_subcode) {}

QByteArray Command::for_telegram(void) const {
    return QByteArray(1, this->code) + QByteArray(1, this->subcode);
}
