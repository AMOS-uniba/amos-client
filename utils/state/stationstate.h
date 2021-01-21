#include "forward.h"

#include <QIcon>

#ifndef STATIONSTATE_H
#define STATIONSTATE_H

class StationState: public State {
private:
    QIcon m_icon;
    QString m_tooltip;
public:
    StationState(unsigned char code, const QString &display_name, const QIcon &icon, const QString &tooltip);
    const QIcon& icon(void) const;
    const QString& tooltip(void) const;
    bool operator==(const StationState &other);
    bool operator!=(const StationState &other);
};

#endif // STATIONSTATE_H
