#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDir>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
private:
    bool m_valid;
    bool m_spectral;
    QDir m_dir;
    QString m_prefix;
    QString m_xml, m_pjpg, m_tjpg, m_mbmp, m_pbmp, m_avi, m_full;
    QDateTime m_timestamp;
    QDateTime m_deferred_until;
    QUuid m_uuid;

    QString try_open(const QString & path, bool required);
    inline QString spectral_string(void) const { return this->is_spectral() ? "spectral" : "all-sky"; };
    void copy_or_move(const QDir & dir, bool keep);
public:
    Sighting(void);
    Sighting(const QDir & dir, const QString & prefix, bool spectral);
    ~Sighting(void) = default;

    qint64 avi_size(void) const;
    inline QDir dir(void) const { return this->m_dir; }
    inline QString str_dir(void) const { return this->m_dir.canonicalPath(); }
    inline const QString & prefix(void) const { return this->m_prefix; }
    inline bool is_spectral(void) const { return this->m_spectral ? true : false; }
    inline QDateTime timestamp(void) const { return this->m_timestamp; }
    inline QDateTime deferred_until(void) const { return this->m_deferred_until; }
    inline double deferred_for(void) const {
        return static_cast<double>((this->deferred_until() - QDateTime::currentDateTimeUtc()).count()) / 1000.0;
    }
    inline bool is_deferred(void) const { return (this->deferred_until() >= QDateTime::currentDateTimeUtc()); }

    QVector<QString> files(void) const;
    QString str(void) const;

    QHttpPart jpg_part(void) const;
    QHttpPart xml_part(void) const;
    QHttpPart json(void) const;

    void debug(void) const;

    void move(const QDir & dir);
    void copy(const QDir & dir);
    void defer(float seconds);
    void discard(void);

    bool hack_Y16(void) const;
};

#endif // SIGHTING_H
