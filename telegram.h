#include "forward.h"

#ifndef TELEGRAM_H
#define TELEGRAM_H


class Telegram {
private:
    constexpr static unsigned char START_BYTE_SLAVE = 0x5A;
    constexpr static unsigned char START_BYTE_MASTER = 0x55;
    constexpr static unsigned char END_BYTE = 0x0D;
    constexpr static unsigned char MAX_LENGTH = 100;

    static unsigned char extract_address(const QByteArray& telegram);
    static unsigned char extract_length(const QByteArray& telegram);

    static unsigned char char_to_hex(unsigned char value);
    static unsigned char hex_to_char(unsigned char value);
    static unsigned char encode_msq(unsigned char value);
    static unsigned char encode_lsq(unsigned char value);

    static unsigned char decode_byte(unsigned char first, unsigned char second);

//    static unsigned char compute_crc(const QByteArray& data);
//    unsigned char compute_crc(void);
public:
    Telegram(const unsigned char address, const QByteArray& message);
    Telegram(const QByteArray& message);

    static bool validate_length(const QByteArray& received) const;
    static bool validate_address(const QByteArray& received) const;
    static bool validate_payload_length(const QByteArray& received) const;
    static bool validate_crc(const QByteArray& received) const;

    QByteArray raw(void) const;



};

#endif // TELEGRAM_H
