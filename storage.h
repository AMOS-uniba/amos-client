#include "forward.h"

#ifndef STORAGE_H
#define STORAGE_H


class Storage {
private:
    QString name;
    QDir directory;
public:
    Storage(const QString& _name, const QDir& _directory);
    QStorageInfo info(void) const;
    QJsonObject json(void) const;
    const QString& get_name(void) const;

    const QDir& get_directory(void) const;
    void set_directory(const QDir& dir);
};


#endif // STORAGE_H
