#include "include.h"

extern Log logger;

Telegram::Telegram(const unsigned char address, const QByteArray& message) {
    this->address = address;
    this->message = message;
}

Telegram::Telegram(const QByteArray& received) {
    unsigned char length = received.length();
    if (length < 8) {
        throw std::runtime_error("Telegram \"\" is too short");
    }
    if (length > Telegram::MAX_LENGTH) {
        throw std::runtime_error("Telegram is too long");
    }

    logger.debug(QString("Telegram length OK (%1)").arg(length));

    unsigned char payload_length = Telegram::extract_length(received);
    if (length != payload_length * 2 + 8) {
        throw RuntimeException(QString("Real length %1 bytes does not match announced payload \
            length %2 characters (total %3 bytes)").arg(length).arg(payload_length).arg(payload_length * 2 + 8));
    }
    logger.debug(QString("Lengths match (%1)").arg(length));

    unsigned char crc = 0;
    for (unsigned char i = 0; i < length - 3; i++) {
        crc += received[i];
    }

    unsigned char received_crc = Telegram::decode_byte(received[length - 3], received[length - 2]);
    if (crc != received_crc) {
        throw RuntimeException(QString("CRCs do not match (computed %1, received %2)").arg(crc).arg(received_crc));
    }
    logger.debug(QString("CRC is OK (%1)").arg(crc));

    for (unsigned char i = 0; i < payload_length; i++) {
        this->message[i] = Telegram::decode_byte(received[5 + 2 * i], received[6 + 2 * i]);
    }
    logger.debug(QString("Decoded message is \"%1\"").arg(QString(this->message)));
}

unsigned char Telegram::extract_address(const QByteArray& received) {
    return Telegram::decode_byte(received[1], received[2]);
}

unsigned char Telegram::extract_length(const QByteArray& received) {
    return Telegram::decode_byte(received[3], received[4]);
}

// Convert a character [0-9A-F] to its hexadecimal value (0-15)
unsigned char Telegram::char_to_hex(unsigned char value) {
    logger.warning(QString("Char to hex %1").arg(value));
    if ((value >= 48) && (value <= 57)) {
        return value - (unsigned char) 48;
    }

    if ((value >= 65) && (value <= 70)) {
        return value - (unsigned char) 55;
    }

    // Fall-through: unknown byte, throw exception
    throw std::range_error(QString("Invalid character to decode: %1").arg(value).toStdString());
}

// Convert a hexadecimal value (0-15) to a character [0-9A-F]
unsigned char Telegram::hex_to_char(unsigned char value) {
    if (value < 10) {
        return value + (unsigned char) 48;
    }

    if (value < 16) {
        return value + (unsigned char) 55;
    }

    // Fall-through: unknown value, throw exception
    throw std::range_error(QString("Invalid value to encode: %1").arg(value).toStdString());
}

// Encode least sigificant quad to char
unsigned char Telegram::encode_lsq(unsigned char value) {
    return Telegram::hex_to_char(value & 0x0F);
}

// Encode most significant quad to char
unsigned char Telegram::encode_msq(unsigned char value) {
    return Telegram::hex_to_char((value & 0xF0) >> 4);
}

QByteArray Telegram::raw(void) const {
    QByteArray buffer(Telegram::MAX_LENGTH, '\0');
    unsigned int i = 0;
    unsigned char crc = 0;
    logger.debug(QString("Address %1, message \"%2\"").arg(this->address).arg(QString(this->message)));

    crc += buffer[0] = Telegram::START_BYTE_MASTER;
    crc += buffer[1] = Telegram::encode_msq(address);
    crc += buffer[2] = Telegram::encode_lsq(address);

    unsigned char length = this->message.length();

    crc += buffer[3] = Telegram::encode_msq(length);
    crc += buffer[4] = Telegram::encode_lsq(length);

    for (unsigned char i = 0; i < length; i++) {
        crc += buffer[5 + 2*i] = Telegram::encode_msq(this->message[i]);
        crc += buffer[6 + 2*i] = Telegram::encode_lsq(this->message[i]);
    }

    i = 5 + 2 * length;

    buffer[i++] = Telegram::encode_msq(crc);
    buffer[i++] = Telegram::encode_lsq(crc);
    buffer[i++] = Telegram::END_BYTE;

    return buffer;
}

// Decode address in hexadecimal format "AB" to the corresponding byte
unsigned char Telegram::decode_byte(unsigned char first, unsigned char second) {
    return (Telegram::char_to_hex(first) << 4) + (Telegram::char_to_hex(second));
}
