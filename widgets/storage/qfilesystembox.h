#ifndef QFILESYSTEMBOX_H
#define QFILESYSTEMBOX_H

#include <QDir>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QGridLayout>
#include <QFileDialog>
#include <QDesktopServices>

#include <QStorageInfo>

#include "logging/eventlogger.h"
#include "utils/sighting.h"


class QFileSystemBox: public QGroupBox {
    Q_OBJECT
protected:
    virtual QString DialogTitle(void) const = 0;
    virtual QString AbortMessage(void) const = 0;
    virtual QString MessageEnabled(void) const = 0;
    virtual QString MessageDirectoryChanged(void) const = 0;

    constexpr static unsigned int ScanInterval = 2000;
    QString path_key(void) const;
    QString enabled_key(void) const;

    bool m_enabled;
    QString m_default_path;
    QDir m_directory;
    QString m_camera;
    QString m_id;

    QCheckBox * m_cb_enabled;
    QLineEdit * m_le_path;
    QPushButton * m_bt_open;
    QPushButton * m_bt_change;
    QProgressBar * m_pb_capacity;
    QGridLayout * m_layout;

    QTimer * m_timer;

    void select_directory(void);

public:
    explicit QFileSystemBox(QWidget * parent = nullptr);

    inline const QString & id(void) const { return this->m_id; };
    inline const QDir & directory(void) const { return this->m_directory; };
    inline const QString & camera(void) const { return this->m_camera; };
    inline const QString full_id(void) const { return QString("%1-%2").arg(this->camera(), this->id()); };

    bool is_enabled(void) const;
    QStorageInfo info(void) const;

public slots:
    void initialize(const QString & camera, const QString & id, const QString & default_path);
    void load_settings(const QSettings * const settings);
    //void save_settings(QSettings * settings) const;

    void scan_info(void);
    virtual void set_directory(const QDir & new_directory);
    virtual void set_enabled(bool enabled);

    void open_in_explorer(void) const;

signals:
    void toggled(bool enabled);
    void directory_changed(const QDir & directory);
    void low_disk_space(void);
};

#endif // QFILESYSTEMBOX_H
