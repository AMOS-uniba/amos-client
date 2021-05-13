#include "forward.h"

#ifndef SERIALPORTSTATE_H
#define SERIALPORTSTATE_H

class SerialPortState: public State {
public:
    SerialPortState(unsigned char code, const QString & display_name);
};

#endif // SERIALPORTSTATE_H
