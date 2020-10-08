#include "include.h"

extern Log logger;

Telegram::Telegram(const unsigned char address, const QByteArray& message) {
    this->address = address;
    this->message = message;
}

Telegram::Telegram(const QByteArray& received) {
    this->validate_length(received);
    this->validate_address(received);
    this->validate_payload_length(received);
    this->validate_crc(received);

    for (unsigned char i = 0; i < payload_length; i++) {
        this->message[i] = Telegram::decode_byte(received[5 + 2 * i], received[6 + 2 * i]);
    }
    logger.debug(QString("Decoded message is \"%1\"").arg(QString(this->message)));
}

void Telegram::validate_length(const QByteArray& received) const {
    unsigned char length = received.length();
    if (length < 8) {
        throw RuntimeException(QString("Telegram \"%1\" is too short").arg(QString(received)));
    }
    if (length > Telegram::MAX_LENGTH) {
        throw RuntimeException("Telegram is too long");
    }
    logger.debug(QString("Telegram length OK (%1)").arg(length));

}

void Telegram::validate_address(const QByteArray& received) const {
    unsigned char received_address = Telegram::extract_address(received);
    if (this->address != received_address) {
        throw RuntimeException(QString("Addresses do not match (should be %1, received %2)").arg(this->address, received_address));
    } else {
        logger.debug(QString("Addresses match (%1)").arg(this->address));
    }
}

void Telegram::validate_payload_length(const QByteArray& received) const {
    unsigned char total_length = received.length();
    unsigned char payload_length = Telegram::extract_length(received);
    if (total_length != payload_length * 2 + 8) {
        throw RuntimeException(QString("Real length %1 bytes does not match announced payload \
            length %2 characters (total %3 bytes)").arg(total_length).arg(payload_length).arg(payload_length * 2 + 8));
    } else {
        logger.debug(QString("Lengths match (total %1 bytes, payload %2 characters)").arg(total_length).arg(payload_length));
    }
}

void Telegram::validate_crc(const QByteArray& received) const {
    unsigned char computed_crc = 0;
    unsigned char length = received.length();
    for (unsigned char i = 0; i < length - 3; i++) {
        computed_crc += received[i];
    }

    unsigned char received_crc = Telegram::decode_byte(received[length - 3], received[length - 2]);
    if (computed_crc != received_crc) {
        throw RuntimeException(QString("CRCs do not match (computed %1, received %2)").arg(computed_crc).arg(received_crc));
    } else {
        logger.debug(QString("CRC is OK (%1)").arg(computed_crc));
    }
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
    throw OutOfRange(QString("Invalid character to decode: %1").arg(value));
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
    throw OutOfRange(QString("Invalid value to encode: %1").arg(value));
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
