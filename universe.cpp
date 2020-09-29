#include "universe.h"

Universe::Universe() {

}

double Universe::mjd(const QDateTime& time) {
    return 40587.0 + ((double) time.toSecsSinceEpoch()) / 86400.0;
}

double Universe::julian_centuries(const QDateTime& time) {
    return (Universe::mjd(time) + Universe::delta_t - MJD_J2000) / 36525.0;
}

Vec3D Universe::compute_sun_ecl(const QDateTime& time) {
    return SunPos(Universe::julian_centuries(time));
}

Vec3D Universe::compute_sun_equ(const QDateTime& time) {
}
