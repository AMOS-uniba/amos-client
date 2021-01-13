#include "forward.h"

#ifndef STORAGE_H
#define STORAGE_H

class Storage {
private:
    QString m_name;
    QDir m_root_directory;
    QDir m_active_directory;
public:
    Storage(const QString &name, const QDir &directory);
    QStorageInfo info(void) const;
    QJsonObject json(void) const;
    const QString& name(void) const;

    const QDir current_directory(void) const;
    const QDir& root_directory(void) const;
    void set_root_directory(const QDir &dir);


public slots:
    void store_sighting(Sighting &sighting, bool del = false) const;
};


#endif // STORAGE_H
