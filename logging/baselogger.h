#include <QObject>
#include <QFile>
#include <QDir>

#ifndef BASELOGGER_H
#define BASELOGGER_H

class BaseLogger: public QObject {
    Q_OBJECT
protected:
    QString m_filename;
    QFile * m_file = nullptr;
    QDir m_directory;

    /** Roll the file over once it grows past MaxSize, keeping the older ones beside it as
     *  `<name>.1` up to `<name>.<Generations>`, `.1` being the most recent.
     *
     *  Nothing trimmed these files before, so a station left running wrote a single log until the
     *  disk objected -- and `state.log` gained a line per heartbeat for as long as it ran. Call it
     *  after writing each line: the size of an open file is one stat, which is nothing against the
     *  formatting the caller has already done to build the line.
    **/
    void rotate_if_oversized(void) const;

    /* 8 MiB live plus five kept, so one log costs at most 48 MiB. Only `events.log` at debug level
     * really grows; `state.log` takes a line per heartbeat and will seldom fill even the first.
     */
    constexpr static qint64 MaxSize = 8 * 1024 * 1024;
    constexpr static int Generations = 5;

public:
    explicit BaseLogger(QObject * parent, const QString & filename);
    ~BaseLogger(void);

    void initialize(void);
    QString filename(void) const;
};

#endif // BASELOGGER_H
