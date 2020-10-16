#include <QObject>
#include <QListWidget>
#include <QTableWidget>
#include <QFile>
#include <QTextStream>

#include "forward.h"

#ifndef LOG_H
#define LOG_H

enum class Level {
    DebugDetail,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

struct LevelInfo {
    QString code;
    QString name;
    Qt::GlobalColor colour;
};

class Log: public QObject {
    Q_OBJECT
private:
    QFile *file = nullptr;
    QTableWidget *display = nullptr;

    const static QMap<Level, LevelInfo> Levels;

    QString format(const QDateTime& timestamp, Level level, const QString& message) const;
    void write(Level level, const QString& message) const;
public:
    explicit Log(QObject *parent, const QString& filename);
    ~Log(void);

    void set_display_widget(QTableWidget* widget);

    void detail(const QString& message) const;
    void debug(const QString& message) const;
    void info(const QString& message) const;
    void warning(const QString& message) const;
    void error(const QString& message) const;
    void fatal(const QString& message) const;
};

#endif // LOG_H
