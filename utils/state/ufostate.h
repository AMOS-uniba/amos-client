#ifndef UFOSTATE_H
#define UFOSTATE_H

#include <QColor>
#include "utils/state/state.h"

class UfoState: public State {
private:
    QColor m_colour;
    bool m_button_enabled;
    QString m_button_text;
public:
    UfoState(unsigned char code, const QString & display_name, QColor colour, bool enable, QString text);

    QColor colour(void) const;
    bool button_enabled(void) const;
    QString button_text(void) const;
};

bool operator==(const UfoState & first, const UfoState & second);
bool operator!=(const UfoState & first, const UfoState & second);

#endif // SERIALPORTSTATE_H
