#include "forward.h"

#ifndef FILESYSTEMSCANNER_H
#define FILESYSTEMSCANNER_H

class FileSystemScanner: public QObject {
    Q_OBJECT

private:
    QDir m_directory;
    QTimer *m_timer;
public:
    FileSystemScanner(const QDir &directory);
    void scan(void) const;

    QDir directory(void) const;

signals:
    void sightings_found(QVector<Sighting> sightings) const;
};

#endif // FILESYSTEMSCANNER_H
