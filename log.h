#include <QObject>
#include <QListWidget>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include "forward.h"

#ifndef LOG_H
#define LOG_H

enum class Level {
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};


class Log: public QObject {
    Q_OBJECT
private:
    QFile *file = nullptr;
    QListWidget *display = nullptr;

    static QMap<Level, QString> Levels;

    QString format(Level level, const QString& message) const;

public:
    explicit Log(QObject *parent, const QString& filename);
    ~Log(void);

    void set_display_widget(QListWidget* widget);

    void debug(const QString& message) const;
    void info(const QString& message) const;

    void write_to_file(Level level, const QString& message) const;
};

#endif // LOG_H
