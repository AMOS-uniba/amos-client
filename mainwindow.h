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

#include "forward.h"

#include "dome.h"

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

private slots:
    void load_settings(void);
    void create_timers(void);

    void on_actionExit_triggered();

    void process_timer(void);
    void request_telegram(void);

    void display_time(void);
    void display_sun_properties(void);
    void display_env_data(void);

    void display_cover_status(void);
    void display_gizmo_status(void);
    void display_storage_status(const Storage& storage, QProgressBar *pb, QLineEdit *le);
    void display_storage_status(void);
    void display_station_config(void);

    void send_heartbeat(void);

    void on_button_send_heartbeat_pressed();

    void on_button_cover_clicked();
    void move_cover(void);

    void on_bt_station_apply_clicked();

    void station_position_edited(void);

    void button_station_toggle(bool enable);

    void on_bt_station_reset_clicked();

    void set_storage(Storage& storage, QLineEdit& edit);

    void on_bt_primary_clicked();
    void on_bt_permanent_clicked();

    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    void on_cb_manual_stateChanged(int enable);
    void on_cb_debug_stateChanged(int debug);

    void on_co_serial_ports_currentIndexChanged(int index);

    void set_icon(int index);
    void icon_activated(QSystemTrayIcon::ActivationReason reason);
    void show_message();
    void message_clicked();

    void on_pushButton_3_clicked();

private:
    QTimer *timer_operation, *timer_cover, *timer_telegram, *timer_heartbeat;
    Ui::MainWindow *ui;
    QSettings *settings;

    double sun_azimuth = 0;
    double sun_altitude = 0;

    Station *station;
    Universe *universe;
    Server *server;

    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *quitAction;
    void create_actions(void);
    void create_tray_icon(void);

    QSystemTrayIcon *tray_icon;
    QMenu *trayIconMenu;

    CommThread comm_thread;

    QList<QSerialPortInfo> serial_ports;
};
#endif // MAINWINDOW_H
