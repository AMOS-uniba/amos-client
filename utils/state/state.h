#ifndef STATE_H
#define STATE_H

#include <QString>

class State {
private:
    unsigned char m_code;
    QString m_display_string;
public:
    State(unsigned char code, const QString & display_name);
    unsigned char code(void) const;
    const QString & display_string(void) const;
};

#endif // STATE_H
