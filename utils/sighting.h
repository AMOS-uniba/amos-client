#include "forward.h"

#include <QFile>
#include <QHttpMultiPart>
#include <QHttpPart>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
private:
    QString m_jpg, m_xml, m_bmp, m_avi;
    QDateTime m_timestamp;

    QVector<QString> m_files;

    void try_open(const QString &path);

    void init_files(const QString &prefix);
public:
    Sighting(const QString &prefix);
    ~Sighting(void);

    QHttpPart jpg_part(void) const;
    QHttpPart xml_part(void) const;

    void move(const QString &prefix);

    const QString& jpg(void) const;
    const QString& xml(void) const;
    const QString& bmp(void) const;
    const QString& avi(void) const;
};

#endif // SIGHTING_H
