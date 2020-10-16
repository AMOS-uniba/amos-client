#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QByteArray>

#include "forward.h"

class Request {
protected:
    unsigned char code;
    QString display_name;
public:
    Request(unsigned char _code, const QString& _display_name);
    QByteArray for_telegram(void) const;
    const QString& get_display_name(void) const;
};

class Command: public Request {
private:
    unsigned char subcode;
public:
    Command(unsigned char _subcode, const QString& _display_name);
    QByteArray for_telegram(void) const;
};

#endif // PROTOCOL_H
