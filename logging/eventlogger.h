#include <QObject>
#include <QListWidget>
#include <QTableWidget>
#include <QFile>
#include <QTextStream>

#include "forward.h"
#include "logging/baselogger.h"

#ifndef LOG_H
#define LOG_H

enum class Level {
    DebugDetail = 10,
    Debug = 8,
    DebugError = 7,
    Info = 6,
    Warning = 4,
    Error = 2,
    Fatal = 0,
};

enum class Concern {
    Generic,
    SerialPort,
    Server,
    Sightings,
    Configuration,
    Automatic,
    Heartbeat,
    UFO,
};

struct LevelInfo {
    QString code;
    QString name;
    Qt::GlobalColor colour;
};

class EventLogger: public BaseLogger {
    Q_OBJECT
private:
    QTableWidget *m_display = nullptr;
    Level logging_level;

    const static QMap<Level, LevelInfo> Levels;
    const static QMap<Concern, QString> Concerns;

    QString format(const QDateTime &timestamp, Level level, const QString &concern, const QString& message) const;
    void write(Level level, Concern concern, const QString &message) const;
public:
    explicit EventLogger(QObject *parent, const QString &filename);

    void set_display_widget(QTableWidget *widget);
    void set_level(Level new_level);

    void detail(const QString &message) const;
    void debug(const QString &message) const;
    void debug_error(const QString &message) const;
    void info(const QString &message) const;
    void warning(const QString &message) const;
    void error(const QString &message) const;
    void fatal(const QString &message) const;

    void detail(Concern concern, const QString &message) const;
    void debug(Concern concern, const QString &message) const;
    void debug_error(Concern concern, const QString &message) const;
    void info(Concern concern, const QString &message) const;
    void warning(Concern concern, const QString &message) const;
    void error(Concern concern, const QString &message) const;
    void fatal(Concern concern, const QString &message) const;
};

#endif // LOG_H
