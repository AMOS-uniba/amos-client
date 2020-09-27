#ifndef UNIVERSE_H
#define UNIVERSE_H

class Universe {
public:
    Universe();
    double get_sun_latitude(void) const;
    double get_sun_longitude(void) const;

    //double get_sun_altitude(const Station& station) const;
};

#endif // UNIVERSE_H
