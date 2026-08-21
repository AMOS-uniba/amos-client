#ifndef QUFOMANAGER_H
#define QUFOMANAGER_H

#include <QGroupBox>
#include <QProcess>

#include "windows.h"
#include "winuser.h"
#include "utils/state/ufostate.h"


QT_FORWARD_DECLARE_CLASS(QStation);

namespace Ui {
    class QUfoManager;
}


class QUfoManager: public QGroupBox {
    Q_OBJECT
private:
    Ui::QUfoManager * ui;

    QTimer * m_timer_check;
    QTimer * m_timer_delay;
    QTimer * m_timer_sequence;
    mutable bool m_start_scheduled;

    /* Starting and stopping UFO used to sleep on the GUI thread -- twice on the way up, up to four
     * times on the way down, plus a SendMessage(WM_CLOSE) that waits on UFO's own message loop. That
     * stalls dome telegram processing, the scanner and the display for up to a second, against the
     * dome's own two-second validity window, and it lands at dusk and dawn. Each step is now driven
     * from m_timer_sequence instead, and the fixed sleeps become bounded polling, which also handles
     * the ordering the sleeps were only guessing at.
     */
    enum class Step {
        Idle,
        FindWindow,         // process started, waiting for UFO's window to appear
        AwaitPopup,         // WM_CLOSE posted, waiting for the confirmation dialog
        AwaitExit,          // dialog dealt with, waiting for the process to go away
        AwaitExitAfterQuit, // WM_QUIT posted, last wait before giving up
    };

    Step m_step = Step::Idle;
    int m_attempts = 0;

    void advance(Step step);
    void run_step(void);
    inline bool is_stopping(void) const { return (this->m_step != Step::Idle) && (this->m_step != Step::FindWindow); };

    mutable HWND m_frame;
    mutable QProcess m_process;
    QString m_path;
    QString m_id;

    bool m_autostart;
    UfoState m_state;
    void update_state(void);

    void start_ufo_inner(void);

    constexpr static int OpenDelay = 20;
    constexpr static int PollInterval = 200;        // ms between sequence steps
    constexpr static int FindWindowAttempts = 15;   // 3 s for UFO's window to appear
    constexpr static int PopupAttempts = 10;        // 2 s for the confirmation dialog
    constexpr static int ExitAttempts = 15;         // 3 s for the process to exit

    constexpr static bool DefaultEnabled = true;
    const static QString DefaultPathAllSky;
    const static QString DefaultPathSpectral;

private slots:
    void on_cb_auto_clicked(bool checked);
    void on_bt_change_clicked();

public:
    const static UfoState Unknown, NotAnExe, NotFound, NotRunning, Starting, Running, Stopping;

    explicit QUfoManager(QWidget * parent = nullptr);
    ~QUfoManager();

    inline const QString & id(void) const { return this->m_id; }

    void set_autostart(bool enable);
    inline bool is_autostart(void) const { return this->m_autostart; }

    void set_path(const QString & path);
    inline const QString & path(void) const { return this->m_path; };

    inline bool is_running(void) const { return this->m_process.state() == QProcess::ProcessState::Running; };
    inline UfoState state(void) const { return this->m_state; };
    QJsonObject json(void) const;

public slots:
    void initialize(const QString & id);
    void load_settings(void);
    void save_settings(void) const;
    void auto_action(bool is_dark, const QDateTime & open_since);

    void start_ufo(unsigned int delay = 0) const;
    void stop_ufo(void);

    void log_state_change(const UfoState & state) const;

signals:
    void started(void);
    void stopped(void);

    void state_changed(const UfoState & state);
};

#endif // QUFOMANAGER_H
