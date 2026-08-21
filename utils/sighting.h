#include <QFile>
#include <QDateTime>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDir>

#ifndef SIGHTING_H
#define SIGHTING_H

class Sighting {
public:
    typedef enum {
        // Raw
        Unprocessed         = 0x00,
        Sent                = 0x01,
        // Remote response
        Accepted            = 0x10,
        Duplicate           = 0x11,
        Rejected            = 0x12,
        UnknownError        = 0x13,
        // Connection problems
        Timeout             = 0x20,
        RemoteHostClosed    = 0x21,
        // Final states
        Stored              = 0x40,
        Quarantined         = 0x41
    } Status;
private:
    bool m_valid;
    bool m_spectral;
    qint64 m_avi_size;
    QDir m_dir;
    QString m_prefix;
    QStringList m_files;
    QString m_full;
    QDateTime m_timestamp;
    QDateTime m_deferred_until;
    Status m_status;

    QString try_open(const QString & path, bool required);
    QHttpPart json(void) const;
    QHttpPart build_part(const QString & filename) const;
    qint64 measure_avi(void) const;
    // Not static: which JPEG is the composite depends on the other files in the sighting
    bool should_send(const QString & path) const;

    static QDateTime parse_timestamp(const QString & path);
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
        return this->m_status == Status::Stored || this->m_status == Status::Quarantined;
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

    QList<QHttpPart> assemble(void) const;

    void debug(void) const;

    bool move(const QDir & dir);
    void defer(float seconds);
    void undefer(void);
};

#endif // SIGHTING_H
