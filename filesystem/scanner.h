#include "forward.h"

#include "filesystem/manager.h"

#ifndef FILESYSTEMSCANNER_H
#define FILESYSTEMSCANNER_H


class FileSystemScanner: public FileSystemManager {
    Q_OBJECT
private:
    QTimer *m_timer;
public:
    FileSystemScanner(const QDir &directory);
    void scan(void) const;

    void set_directory(const QDir &dir) override;
signals:
    void sightings_found(QVector<Sighting> sightings) const;
};

#endif // FILESYSTEMSCANNER_H
