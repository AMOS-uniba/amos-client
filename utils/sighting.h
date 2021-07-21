#include "forward.h"

#include <QFile>
#include <QHttpMultiPart>
#include <QHttpPart>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
private:
    bool m_spectral;
    QString m_dir, m_prefix, m_jpg, m_jpt, m_xml, m_bmp, m_avi;
    QDateTime m_timestamp;

    QVector<QString> m_files;

    QString try_open(const QString & path, bool require);
public:
    Sighting(const QString & dir, const QString & prefix, bool spectral);
    ~Sighting(void);

    qint64 avi_size(void) const;
    inline QString dir(void) const { return this->m_dir; }
    inline QString prefix(void) const { return this->m_prefix; }
    inline bool is_spectral(void) const { return this->m_spectral; }

    QHttpPart jpg_part(void) const;
    QHttpPart xml_part(void) const;
    QHttpPart json(void) const;

    void debug(void) const;

    void move(const QString & dir);
    void copy(const QString & dir) const;

    bool hack_Y16(void) const;
};

#endif // SIGHTING_H
