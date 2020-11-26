#include "forward.h"

#include <QFile>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
private:
    QString m_jpg, m_xml;
    QDateTime m_timestamp;
public:
    Sighting(const QString &jpg, const QString &xml);
};

#endif // SIGHTING_H
