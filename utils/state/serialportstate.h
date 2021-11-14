#include "forward.h"

#ifndef SERIALPORTSTATE_H
#define SERIALPORTSTATE_H

class SerialPortState: public State {
private:
    QColor m_colour;
public:
    SerialPortState(void);
    SerialPortState(unsigned char code, const QString & display_name, QColor colour);
    inline QColor colour(void) const { return this->m_colour; }
};

#endif // SERIALPORTSTATE_H
