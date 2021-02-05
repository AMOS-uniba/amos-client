#include "forward.h"

#include <QFile>
#include <QHttpMultiPart>
#include <QHttpPart>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
private:
    QString m_prefix, m_jpg, m_xml, m_bmp, m_avi;
    QDateTime m_timestamp;

    QVector<QString> m_files;

    QString try_open(const QString &path, bool require);
public:
    Sighting(const QString &prefix);
    ~Sighting(void);

    qint64 avi_size(void) const;

    QHttpPart jpg_part(void) const;
    QHttpPart xml_part(void) const;
    QHttpPart json_metadata(void) const;

    void move(const QString &prefix);
    void copy(const QString &prefix) const;

    bool hack_Y16(void) const;
};

#endif // SIGHTING_H
