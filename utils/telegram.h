#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <QByteArray>

class Telegram {
private:
    unsigned char m_address;
    QByteArray m_message;

    constexpr static unsigned char StartByteSlave = 0x5A;
    constexpr static unsigned char StartByteMaster = 0x55;
    constexpr static unsigned char EndByte = 0x0D;
    constexpr static unsigned char MaxLength = 100;

    static unsigned char char_to_hex(unsigned char value);
    static unsigned char hex_to_char(unsigned char value);
    static unsigned char encode_msq(unsigned char value);
    static unsigned char encode_lsq(unsigned char value);

    static unsigned char decode_byte(unsigned char first, unsigned char second);

public:
    Telegram(const unsigned char address, const QByteArray & message);
    Telegram(const QByteArray & message);

    QByteArray compose(void) const;
    inline QByteArray get_message(void) const { return this->m_message; };
};

#endif // TELEGRAM_H
