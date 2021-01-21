#include "forward.h"

#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

class FileSystemManager: public QObject {
    Q_OBJECT
protected:
    QDir m_directory;
public:
    FileSystemManager(const QDir &directory);
    QStorageInfo info(void) const;

    virtual void set_directory(const QDir &dir);
    QDir directory(void) const;

    void open_in_explorer(void) const;
signals:
    void directory_set(const QDir &dir);
};

#endif // FILESYSTEMMANAGER_H
