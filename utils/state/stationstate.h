#ifndef STATIONSTATE_H
#define STATIONSTATE_H

#include <QIcon>

#include "utils/state/state.h"

enum class Icon {
    Observing,
    NotObserving,
    Daylight,
    Manual,
    Failure,
};

class StationState: public State {
private:
    Icon m_icon;
    QString m_tooltip;
public:
    StationState(unsigned char code, const QString & display_name, Icon icon, const QString & tooltip);
    Icon icon(void) const;
    const QString & tooltip(void) const;
    bool operator==(const StationState & other);
    bool operator!=(const StationState & other);
};

#endif // STATIONSTATE_H
