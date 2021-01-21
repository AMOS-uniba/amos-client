#include "include.h"

StationState::StationState(unsigned char code, const QString &display_string, const QIcon &icon, const QString &tooltip):
    State(code, display_string),
    m_icon(icon),
    m_tooltip(tooltip) {}

const QIcon& StationState::icon(void) const { return this->m_icon; }
const QString& StationState::tooltip(void) const { return this->m_tooltip; }

bool StationState::operator==(const StationState &other) { return (this->code() == other.code()); }
bool StationState::operator!=(const StationState &other) { return (!(this->code() == other.code())); }

