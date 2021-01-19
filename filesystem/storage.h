#include "forward.h"

#include "filesystem/manager.h"

#ifndef STORAGE_H
#define STORAGE_H

class Storage: public FileSystemManager {
private:
    QString m_name;
public:
    Storage(const QString &name, const QDir &directory);
    QJsonObject json(void) const;
    const QString& name(void) const;

    const QDir current_directory(const QDateTime &datetime = QDateTime::currentDateTimeUtc()) const;

    void set_directory(const QDir &dir) override;
public slots:
    void store_sighting(Sighting &sighting, bool del = false) const;
};


#endif // STORAGE_H
