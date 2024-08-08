#include "forward.h"

#include <QFile>
#include <QHttpMultiPart>
#include <QHttpPart>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
private:
    bool m_spectral;
    QString m_dir, m_prefix;
    QString m_xml, m_pjpg, m_tjpg, m_mbmp, m_pbmp, m_avi;
    QDateTime m_timestamp;
    QUuid m_uuid;

    QVector<QString> m_files;

    QString try_open(const QString & path, bool required);
public:
    Sighting(const QString & dir, const QString & prefix, bool spectral);
    ~Sighting(void);

    qint64 avi_size(void) const;
    inline QString dir(void) const { return this->m_dir; }
    inline QString prefix(void) const { return this->m_prefix; }
    inline bool is_spectral(void) const { return this->m_spectral; }
    inline QDateTime timestamp(void) const { return this->m_timestamp; }

    QHttpPart jpg_part(void) const;
    QHttpPart xml_part(void) const;
    QHttpPart json(void) const;

    void debug(void) const;

    void move(const QString & dir);
    void copy(const QString & dir) const;

    bool hack_Y16(void) const;
};

#endif // SIGHTING_H
