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
    const QString& get_name(void) const;

    const QDir& get_directory(void) const;
    void set_directory(const QDir &dir);

    QVector<Sighting> list_new_sightings(void);
    void move_sighting(Sighting& sighting);
    void copy_sighting(Sighting& sighting);

public slots:

signals:
    void sighting_found(void) const;
};


#endif // STORAGE_H
