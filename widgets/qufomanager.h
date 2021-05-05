#ifndef QUFOMANAGER_H
#define QUFOMANAGER_H

#include <QGroupBox>
#include <QProcess>

#include "windows.h"
#include "winuser.h"
#include "process.h"
#include "widgets/qstation.h"

namespace Ui {
    class QUfoManager;
}


enum class UfoState {
    NotFound,
    NotExe,
    NotRunning,
    Starting,
    Running,
};


class QUfoManager: public QGroupBox {
    Q_OBJECT
private:
    Ui::QUfoManager * ui;

    HWND m_frame;
    bool m_autostart;
    QProcess m_process;
    QString m_path;
    qint64 m_pid;
    const QStation * m_station;

    UfoState m_state;
    void update_state(const UfoState new_state);

    //void update_state(QProcess::ProcessState new_state);

private slots:
    void on_cb_auto_clicked(bool checked);
    void on_bt_change_clicked();

public:
    explicit QUfoManager(QWidget * parent = nullptr);
    ~QUfoManager();

    void set_station(const QStation * const station);

    void set_autostart(bool enable);
    bool is_autostart(void) const;

    void set_path(const QString &path);
    const QString& path(void) const;

    bool is_running(void) const;
    qint64 pid(void) const;

    void start_ufo(void);
    void stop_ufo(void);

    UfoState state(void) const;

public slots:
    void initialize(const QStation * const station);
    void load_settings(void);
    void save_settings(void) const;
    void auto_action(bool is_dark);

signals:
    void started(void) const;
    void stopped(void) const;
};

#endif // QUFOMANAGER_H
