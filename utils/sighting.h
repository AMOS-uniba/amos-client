#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDir>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
public:
    typedef enum {
        Unprocessed = 0,
        Sent = 1,
        Deferred = 2,
        Duplicate = 10,
        Accepted = 20,
        Rejected = 21,
        UnknownStation = 30,
        Stored = 99,
        Discarded = 150,
    } Status;
private:
    bool m_valid;
    bool m_spectral;
    qint64 m_avi_size;
    QDir m_dir;
    QString m_prefix;
    QString m_xml, m_pjpg, m_tjpg, m_mbmp, m_pbmp, m_avi, m_full;
    QDateTime m_timestamp;
    QDateTime m_deferred_until;
    QUuid m_uuid;
    Status m_status;

    QString try_open(const QString & path, bool required);
    void copy_or_move(const QDir & dir, bool keep);
public:
    Sighting(void);
    Sighting(const QDir & dir, const QString & prefix, bool spectral);
    ~Sighting(void) = default;

    inline QDir dir(void) const { return this->m_dir; }
    inline const QString & prefix(void) const { return this->m_prefix; }
    inline QDateTime timestamp(void) const { return this->m_timestamp; }
    inline qint64 avi_size(void) const { return this->m_avi_size; }
    inline QString spectral_string(void) const { return this->is_spectral() ? "spectral" : "all-sky"; };
    inline QString dir_string(void) const { return this->m_dir.canonicalPath(); }
    inline bool is_spectral(void) const { return this->m_spectral ? true : false; }
    inline bool is_deferred(void) const { return (this->deferred_until() >= QDateTime::currentDateTimeUtc()); }
    inline bool is_finished(void) const {
        return this->m_status == Status::Stored || this->m_status == Status::Discarded;
    }
    inline bool is_processed(void) const {
        return !(this->m_status == Status::Unprocessed || this->m_status == Status::Sent);
    }
    inline QDateTime deferred_until(void) const { return this->m_deferred_until; }
    void set_status(Status new_status);

    double deferred_for(void) const;

    inline Status status(void) const { return this->m_status; }
    QVector<QString> files(void) const;
    QString str(void) const;
    QString status_string(void) const;

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
