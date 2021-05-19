#ifndef QUFOMANAGER_H
#define QUFOMANAGER_H

#include <QGroupBox>
#include <QProcess>

#include "windows.h"
#include "winuser.h"
#include "process.h"
#include "widgets/qstation.h"
#include "utils/state/ufostate.h"

namespace Ui {
    class QUfoManager;
}


class QUfoManager: public QGroupBox {
    Q_OBJECT
private:
    Ui::QUfoManager * ui;

    QTimer * m_timer_check;

    mutable HWND m_frame;
    mutable QProcess m_process;
    QString m_path;

    bool m_autostart;
    UfoState m_state;
    void update_state(void);

private slots:
    void on_cb_auto_clicked(bool checked);
    void on_bt_change_clicked();

public:
    const static UfoState Unknown, NotAnExe, NotFound, NotRunning, Starting, Running;

    explicit QUfoManager(QWidget * parent = nullptr);
    ~QUfoManager();

    void set_autostart(bool enable);
    bool is_autostart(void) const;

    void set_path(const QString &path);
    const QString & path(void) const;

    bool is_running(void) const;

    UfoState state(void) const;

    QJsonObject json(void) const;

public slots:
    void initialize(void);
    void load_settings(void);
    void save_settings(void) const;
    void auto_action(bool is_dark) const;

    void start_ufo(void) const;
    void stop_ufo(void) const;

    void log_state_change(const UfoState & state) const;

signals:
    void started(void) const;
    void stopped(void) const;

    void state_changed(UfoState state) const;
};

#endif // QUFOMANAGER_H
