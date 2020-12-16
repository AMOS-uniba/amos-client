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

private:
    QTimer *timer_display, *timer_heartbeat, *timer_watchdog;
    Ui::MainWindow *ui;
    QSettings *settings;

    double sun_azimuth = 0;
    double sun_altitude = 0;

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

    void set_storage(Storage *storage, QLineEdit *edit);
    void display_storage_status(const Storage *storage, QProgressBar *pb, QLineEdit *le);

    void display_S_state_bit(bool value, QLabel *label, const QString &on, const QString &off, const QString &colour_on, const QString &colour_off);
    void display_device_button(QPushButton *button, bool on);
    void display_sensor_value(QLabel *label, float value, const QString& unit);

    //CommThread comm_thread;

    QList<QSerialPortInfo> serial_ports;

private slots:
    void load_settings(void);
    void create_timers(void);
    void init_serial_ports(void);

    void on_actionExit_triggered();

    void process_display_timer(void);
    void process_watchdog_timer(void);

    void display_time(void);
    void display_sun_properties(void);

    void display_window_title(void);

    void display_serial_port_info(void);
    void display_basic_data(void);
    void display_env_data(void);
    void display_shaft_position(void);
    void display_device_buttons_state(void);

    void display_cover_status(void);
    void display_storage_status(void);
    void display_station_config(void);
    void display_ufo_state(void);

    void send_heartbeat(void);
    void on_button_send_heartbeat_pressed();

    void button_station_toggle(bool enable);
    void on_bt_station_reset_clicked();
    void on_bt_station_apply_clicked();
    void on_station_edited(void);

    void on_bt_primary_clicked();
    void on_bt_permanent_clicked();

    void on_cb_manual_stateChanged(int enable);
    void on_cb_debug_stateChanged(int debug);

    void on_co_serial_ports_currentIndexChanged(int index);

    // Tray and messaging
    void set_icon(const StationState &state);
    void icon_activated(QSystemTrayIcon::ActivationReason reason);
    void show_message();
    void message_clicked();

    void on_bt_lens_heating_clicked();
    void on_bt_intensifier_clicked();
    void on_bt_fan_clicked();

    void on_bt_cover_open_clicked();
    void on_bt_cover_close_clicked();

    void on_cb_safety_override_stateChanged(int arg1);

    void on_actionManual_control_triggered();

    void on_bt_change_ufo_clicked();
    void on_checkBox_stateChanged(int arg1);
};
#endif // MAINWINDOW_H
