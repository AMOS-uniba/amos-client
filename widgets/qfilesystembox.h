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
#include "utils/exception.h"
#include "utils/sighting.h"


class QFileSystemBox: public QGroupBox {
    Q_OBJECT
protected:
    virtual QString DialogTitle(void) const = 0;
    virtual QString AbortMessage(void) const = 0;

    bool m_enabled;
    QDir m_directory;

    QCheckBox *m_cb_enabled;
    QLineEdit *m_le_path;
    QPushButton *m_bt_open, *m_bt_change;
    QProgressBar *m_pb_capacity;
    QGridLayout *m_layout;

    QTimer *m_timer;

    void select_directory(void);

public:
    explicit QFileSystemBox(QWidget * parent = nullptr);

    bool is_enabled(void) const;
    QStorageInfo info(void) const;

public slots:
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
