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
    QString m_station;
    int m_counter;
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

public:
    Sighting(void);
    Sighting(const QDir & dir, const QString & prefix, bool spectral, const QStringList & files);
    ~Sighting(void) = default;

    /** Suffix tests. Case-insensitive on purpose: the file system is, UFO is not consistent
     *  about it, and a mismatch here used to mean a sighting was sent with no metadata at all.
    **/
    static bool has_suffix(const QString & filename, const QString & suffix);
    static bool is_metadata(const QString & filename);

    /** Everything a detector encodes in the name of its files.
     *  UFO writes "M<yyyyMMdd_hhmmss>_<station>_" and has no counter, because it cannot tell two
     *  events in the same second apart. Kvant writes "<yyyyMMdd_hhmmss><sep><station><sep><NNNNN>",
     *  with either separator being '-' or '_' depending on its version, and the counter is precisely
     *  how it distinguishes events sharing a second.
    **/
    struct Name {
        QDateTime timestamp;
        QString station;
        int counter = 0;
    };
    static Name parse_name(const QString & prefix);

    // Whether this sighting carries a metadata file at all; an image delivered before its
    // reduction has run does not, and is expected to come back with a null filename.
    bool has_metadata(void) const;

    inline QDir dir(void) const { return this->m_dir; }
    inline const QString & prefix(void) const { return this->m_prefix; }
    inline QDateTime timestamp(void) const { return this->m_timestamp; }
    inline const QString & station(void) const { return this->m_station; }
    inline int counter(void) const { return this->m_counter; }
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
