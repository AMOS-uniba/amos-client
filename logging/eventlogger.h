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

struct LevelInfo {
    QString code;
    QString name;
    Qt::GlobalColor colour;
};

enum class Concern {
    Generic,        // Default
    SerialPort,     // Connection to the dome
    Server,         // Connection to server
    Sightings,      // Sighting objects
    Configuration,  // User and file configuration
    Operation,      // Manual operation
    Automatic,      // Automatic actions
    Heartbeat,      // Heartbeats
    UFO,            // UFO manager
    Storage,        // Scanned directories and storage
};

struct ConcernInfo {
    QString code;
    QString name;
    QString caption;
};

class EventLogger: public BaseLogger {
    Q_OBJECT
private:
    QTableWidget *m_display = nullptr;
    Level logging_level;

    const static QMap<Level, LevelInfo> Levels;

    QMap<Concern, bool> debug_visible;

    QString format(const QDateTime &timestamp, Level level, const QString &concern, const QString& message) const;
    void write(Level level, Concern concern, const QString &message) const;
public:
    explicit EventLogger(QObject *parent, const QString &filename);

    void set_display_widget(QTableWidget *widget);
    void set_level(Level new_level);

    void detail(Concern concern, const QString &message) const;
    void debug(Concern concern, const QString &message) const;
    void debug_error(Concern concern, const QString &message) const;
    void info(Concern concern, const QString &message) const;
    void warning(Concern concern, const QString &message) const;
    void error(Concern concern, const QString &message) const;
    void fatal(Concern concern, const QString &message) const;

    const static QMap<Concern, ConcernInfo> Concerns;
    void set_debug_visible(Concern concern, bool visible);
    bool is_debug_visible(Concern concern) const;
};

#endif // LOG_H
