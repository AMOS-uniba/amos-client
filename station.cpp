#include "station.h"

Station::Station() {

}

double Station::get_sun_altitude(void) const {
    auto now = QDateTime::currentDateTimeUtc();

    return 43;
}

double Station::get_sun_azimuth(void) const {
    auto now = QDateTime::currentDateTimeUtc();

    return 180;
}
