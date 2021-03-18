#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QTimer>
#include <QSerialPort>
#include <QSettings>
#include <QMessageLogger>
#include <QFileDialog>
#include <QLineEdit>
#include <QProgressBar>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QSerialPortInfo>
#include <QDesktopServices>

#include "forward.h"

#include "utils/ufomanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
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
    QTimer *timer_display, *timer_heartbeat, *timer_watchdog;
    Ui::MainWindow *ui;

    Station *station = nullptr;
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

    //CommThread comm_thread;

    QList<QSerialPortInfo> serial_ports;

private slots:
    void load_settings(void);
    void load_settings_storage(void);
    void load_settings_station(void);

    void create_timers(void);

    // Menu actions
    void on_action_exit_triggered();
    void on_action_manual_triggered();
    void on_action_logging_options_triggered();

    void process_display_timer(void);
    void process_watchdog_timer(void);

    // Display
    void display_time(void);

    void display_window_title(void);

    void display_cover_status(void);
    void display_station_config(void);
    void display_ufo_settings(void);
    void display_ufo_state(void);

    void slot_station_edited(void);

    void heartbeat(void);

    void button_station_toggle(bool changed);
    void on_bt_station_reset_clicked();
    void on_bt_station_apply_clicked();

    void on_cb_manual_stateChanged(int manual);
    void on_cb_debug_stateChanged(int debug);

    // Tray and messaging
    void set_icon(const StationState &state);
    void icon_activated(QSystemTrayIcon::ActivationReason reason);
    void show_message();
    void message_clicked();

    // Dome control slots
    void on_cb_safety_override_stateChanged(int arg1);

    void on_bt_change_ufo_clicked();
    void on_cb_ufo_auto_stateChanged(int arg1);
    void on_bt_ufo_clicked();

    void on_action_open_log_triggered();
    void on_action_open_config_triggered();
    void on_action_open_stat_triggered();
    void on_action_debug_triggered();
};
#endif // MAINWINDOW_H
