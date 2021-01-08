#include "forward.h"

#ifndef DISKMANAGER_H
#define DISKMANAGER_H

class DiskManager {
    Q_OBJECT;
private:
    Storage *primary, *permanent;
public:
    DiskManager();

    QVector<Sighting> scan_primary(void) const;

    bool store_to_permanent(const Sighting& sighting);
};

#endif // DISKMANAGER_H
