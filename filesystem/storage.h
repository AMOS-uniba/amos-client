#include "forward.h"

#include "filesystem/manager.h"

#ifndef STORAGE_H
#define STORAGE_H

class Storage: public FileSystemManager {
    Q_OBJECT
private:
    QString m_name;
    bool m_enabled;

public:
    Storage(const QString &name, const QDir &directory);
    QJsonObject json(void) const;
    const QString& name(void) const;
    const QDir current_directory(const QDateTime &datetime = QDateTime::currentDateTimeUtc()) const;

public slots:
    void set_directory(const QDir &dir) override;
    void store_sighting(Sighting &sighting, bool del = false) const;
    void set_enabled(bool enabled);

signals:
    void toggled(bool enabled);
};


#endif // STORAGE_H
