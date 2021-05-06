#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QMessageLogger>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QCloseEvent>

#include "forward.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE


class MainWindow: public QMainWindow {
    Q_OBJECT    
protected:
    void closeEvent(QCloseEvent *event) override;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static QString format_duration(unsigned int seconds);

private:
    QTimer *timer_display;
    Ui::MainWindow *ui;

    Universe *universe = nullptr;

    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *quitAction;
    void create_actions(void);
    void create_tray_icon(void);

    QSystemTrayIcon *tray_icon;
    QMenu *trayIconMenu;

    QMap<Icon, QIcon> icons;
    const QDateTime m_start_time;

    //CommThread comm_thread;

private slots:
    void load_settings(void);

    void create_timers(void);
    void process_display_timer(void);

    // Display
    void on_cb_debug_stateChanged(int debug);
    void display_time(void);
    void display_window_title(void);

    // Tray and messaging
    void set_icon(const StationState &state);
    void icon_activated(QSystemTrayIcon::ActivationReason reason);
    void show_message();
    void message_clicked();

    // Menu actions
    void on_action_exit_triggered();
    void on_action_manual_triggered();
    void on_action_logging_options_triggered();

    void on_action_open_log_triggered();
    void on_action_open_config_triggered();
    void on_action_open_stat_triggered();
    void on_action_debug_triggered();
};
#endif // MAINWINDOW_H
