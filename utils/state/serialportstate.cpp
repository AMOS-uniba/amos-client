#include "serialportstate.h"

SerialPortState::SerialPortState(unsigned char code, const QString & display_string, QColor colour):
    State(code, display_string),
    m_colour(colour)
{}
