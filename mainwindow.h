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

#include "dome.h"
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

    void display_S_state_bit(bool value, QLabel *label, const QString &on, const QString &off, const QString &colour_on, const QString &colour_off);
    void display_device_button(QPushButton *button, bool on);
    void display_sensor_value(QLabel *label, float value, const QString &unit = "");

    QMap<Icon, QIcon> icons;

    //CommThread comm_thread;

    QList<QSerialPortInfo> serial_ports;

private slots:
    void load_settings(void);
    void load_settings_storage(void);
    void load_settings_station(void);

    void create_timers(void);
    void display_serial_ports(void);

    // Menu actions
    void on_action_exit_triggered();
    void on_action_manual_triggered();
    void on_action_logging_options_triggered();

    void process_display_timer(void);
    void process_watchdog_timer(void);

    // Display
    void display_time(void);

    void display_window_title(void);

    void display_serial_port_info(void);
    void display_basic_data(void);
    void display_env_data(void);
    void display_shaft_position(void);
    void display_device_buttons_state(void);

    void display_cover_status(void);
    void display_station_config(void);
    void display_ufo_settings(void);
    void display_ufo_state(void);

    void slot_station_edited(void);

    void heartbeat(void);
    void on_button_send_heartbeat_pressed();

    void button_station_toggle(bool changed);
    void on_bt_station_reset_clicked();
    void on_bt_station_apply_clicked();

    void on_cb_manual_stateChanged(int manual);
    void on_cb_debug_stateChanged(int debug);
    void on_co_serial_ports_activated(int index);

    // Tray and messaging
    void set_icon(const StationState &state);
    void icon_activated(QSystemTrayIcon::ActivationReason reason);
    void show_message();
    void message_clicked();

    // Dome control slots
    void on_bt_lens_heating_clicked();
    void on_bt_intensifier_clicked();
    void on_bt_fan_clicked();

    void on_bt_cover_open_clicked();
    void on_bt_cover_close_clicked();

    void on_cb_safety_override_stateChanged(int arg1);

    void on_bt_change_ufo_clicked();
    void on_cb_ufo_auto_stateChanged(int arg1);
    void on_bt_ufo_clicked();

    void on_action_open_log_triggered();
    void on_action_open_config_triggered();
    void on_action_open_stat_triggered();
};
#endif // MAINWINDOW_H
