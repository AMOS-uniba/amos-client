#include "ufostate.h"

UfoState::UfoState(unsigned char code, const QString & display_string, QColor colour, bool enable_button, QString button_text):
    State(code, display_string),
    m_colour(colour),
    m_button_enabled(enable_button),
    m_button_text(button_text)
{}

QColor UfoState::colour(void) const { return this->m_colour; }
bool UfoState::button_enabled(void) const { return this->m_button_enabled; }
QString UfoState::button_text(void) const { return this->m_button_text; }

bool operator==(const UfoState & first, const UfoState & second) { return (first.code() == second.code()); }
bool operator!=(const UfoState & first, const UfoState & second) { return (first.code() != second.code()); }
