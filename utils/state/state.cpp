#include "state.h"

State::State(unsigned char code, const QString &display_string):
    m_code(code),
    m_display_string(display_string) {}

unsigned char State::code(void) const { return this->m_code; }
const QString& State::display_string(void) const { return this->m_display_string; }
