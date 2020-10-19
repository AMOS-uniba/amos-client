#include "forward.h"

#ifndef TELEGRAM_H
#define TELEGRAM_H


class Telegram {
private:
    unsigned char address;
    QByteArray message;

    constexpr static unsigned char START_BYTE_SLAVE = 0x5A;
    constexpr static unsigned char START_BYTE_MASTER = 0x55;
    constexpr static unsigned char END_BYTE = 0x0D;
    constexpr static unsigned char MAX_LENGTH = 100;

    static unsigned char char_to_hex(unsigned char value);
    static unsigned char hex_to_char(unsigned char value);
    static unsigned char encode_msq(unsigned char value);
    static unsigned char encode_lsq(unsigned char value);

    static unsigned char decode_byte(unsigned char first, unsigned char second);
public:
    Telegram(const unsigned char address, const QByteArray& message);
    Telegram(const QByteArray& message);

    QByteArray compose(void) const;
    QByteArray get_message(void) const;
};

#endif // TELEGRAM_H
