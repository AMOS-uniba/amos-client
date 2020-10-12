#include "include.h"

extern Log logger;

Telegram::Telegram(const unsigned char address, const QByteArray& message): address(address), message(message) {}

Telegram::Telegram(const QByteArray& received) {
    // Check length of the message
    unsigned char length = received.length();
    if ((length < 8) || (length > Telegram::MAX_LENGTH)) {
        throw MalformedTelegram(QString("Telegram has wrong length (%1 bytes)").arg(length));
    }

    // Compare claimed and real length
    unsigned char payload_length = Telegram::decode_byte(received[3], received[4]);
    if (length != payload_length * 2 + 8) {
        throw MalformedTelegram(QString("Real length %1 does not match claimed length %2 (%3 bytes)")
                                .arg(length).arg(payload_length).arg(payload_length * 2 + 8));
    }

    // Compare claimed and computed CRC
    unsigned char crc_received = Telegram::decode_byte(received[length - 3], received[length - 2]);
    unsigned char crc_computed = 0;
    for (unsigned char i = 0; i < length -3; i++) {
        crc_computed += received[i];
    }
    if (crc_computed != crc_received) {
        throw MalformedTelegram(QString("CRCs do not match (computed %1, received %2)").arg(crc_computed).arg(crc_received));
    }

    // Check first and last bytes of the message
    if ((received[0] != Telegram::START_BYTE_SLAVE) && (received[0] != Telegram::START_BYTE_MASTER)) {
        throw MalformedTelegram(QString("Incorrect start byte 0x%1").arg(received[0], 2, 16, QChar('0')));
    }

    if (received[length - 1] != Telegram::END_BYTE) {
        throw MalformedTelegram(QString("Incorrect end byte 0x%1").arg((int) received[length - 1], 2, 16, QChar('0')));
    }

    // Finally extract address and payload
    this->address = Telegram::decode_byte(received[1], received[2]);
    this->message.resize(payload_length);
    for (unsigned char i = 0; i < payload_length; i++) {
        this->message[i] = Telegram::decode_byte(received[5 + 2 * i], received[6 + 2 * i]);
    }
}

// Convert a character [0-9A-F] to its hexadecimal value (0-15)
unsigned char Telegram::char_to_hex(unsigned char value) {
    if ((value >= 48) && (value <= 57)) {
        return value - (unsigned char) 48;
    }

    if ((value >= 65) && (value <= 70)) {
        return value - (unsigned char) 55;
    }

    // Fall-through: unknown byte, throw exception
    throw EncodingError(QString("Invalid character to decode: %1").arg(value));
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
    throw EncodingError(QString("Invalid value to encode: %1").arg(value));
}

// Encode least sigificant quad to char
unsigned char Telegram::encode_lsq(unsigned char value) {
    return Telegram::hex_to_char(value & 0x0F);
}

// Encode most significant quad to char
unsigned char Telegram::encode_msq(unsigned char value) {
    return Telegram::hex_to_char((value & 0xF0) >> 4);
}

// Compose a byte array to be sent over the serial port
QByteArray Telegram::compose(void) const {
    unsigned char length = this->message.length();
    QByteArray buffer(length * 2 + 8, '\0');
    unsigned int i = 0;
    unsigned char crc = 0;
    logger.debug(QString("Address 0x%1, message \"%2\"").arg(this->address, 2, 16, QChar('0')).arg(QString(this->message)));

    crc += buffer[0] = Telegram::START_BYTE_MASTER;
    crc += buffer[1] = Telegram::encode_msq(address);
    crc += buffer[2] = Telegram::encode_lsq(address);


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
