#include "forward.h"

#ifndef BASELOGGER_H
#define BASELOGGER_H

class BaseLogger: public QObject {
    Q_OBJECT
protected:
    QString m_filename;
    QFile *m_file = nullptr;
    QDir m_directory;

public:
    explicit BaseLogger(QObject *parent, const QString &filename);
    void initialize(void);

    QString filename(void) const;
    ~BaseLogger(void);
};

#endif // BASELOGGER_H
