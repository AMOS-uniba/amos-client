#include "forward.h"

#include <QFile>
#include <QHttpMultiPart>
#include <QHttpPart>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
private:
    QString m_jpg, m_xml;
    QDateTime m_timestamp;
public:
    Sighting(const QString &jpg, const QString &xml);
    QHttpPart jpg_part(void) const;
    QHttpPart xml_part(void) const;

    const QString& jpg(void) const;
    const QString& xml(void) const;
};

#endif // SIGHTING_H
