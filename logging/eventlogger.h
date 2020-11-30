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

class EventLogger: public BaseLogger {
    Q_OBJECT
private:
    QTableWidget *m_display = nullptr;
    Level logging_level;

    const static QMap<Level, LevelInfo> Levels;

    QString format(const QDateTime &timestamp, Level level, const QString& message) const;
    void write(Level level, const QString &message) const;
public:
    explicit EventLogger(QObject *parent, const QString &filename);
    ~EventLogger(void);

    void set_display_widget(QTableWidget *widget);
    void set_level(Level new_level);

    void detail(const QString &message) const;
    void debug(const QString &message) const;
    void debug_error(const QString &message) const;
    void info(const QString &message) const;
    void warning(const QString &message) const;
    void error(const QString &message) const;
    void fatal(const QString &message) const;
};

#endif // LOG_H
