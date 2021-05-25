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

public:
    explicit BaseLogger(QObject * parent, const QString & filename);
    ~BaseLogger(void);

    void initialize(void);
    QString filename(void) const;
};

#endif // BASELOGGER_H
