#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QByteArray>

class Request {
protected:
    unsigned char m_code;
    QString m_display_name;

public:
    Request(unsigned char code, const QString & display_name);
    QByteArray for_telegram(void) const;
    const QString & display_name(void) const;
};

class Command: public Request {
private:
    unsigned char m_subcode;

public:
    Command(unsigned char subcode, const QString & display_name);
    QByteArray for_telegram(void) const;
};

#endif // PROTOCOL_H
