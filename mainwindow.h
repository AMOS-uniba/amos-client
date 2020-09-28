#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <random>

#include <QMainWindow>
#include <QMap>
#include <QTimer>
#include <QSerialPort>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>

#include "station.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    bool manual = false;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void load_settings(void);
    void create_timers(void);

    void on_actionExit_triggered();

    void process_timer(void);
    void request_telegram(void);

    void display_sun_properties(void);
    void display_env_data(void);
    void send_heartbeat(void);

    void heartbeat_ok(QNetworkReply *reply);
    void heartbeat_error(QNetworkReply::NetworkError error);
    void heartbeat_response(void);

    void on_button_send_heartbeat_pressed();

    void on_button_cover_clicked();
    void move_cover(void);

    void on_button_station_accept_clicked();

    void log_debug(const QString& message);
    void log_error(const QString& message);

    void on_checkbox_manual_stateChanged(int arg1);

private:
    QTimer *timer_operation, *timer_cover, *timer_telegram, *timer_heartbeat;
    QNetworkAccessManager *network_manager;
    Ui::MainWindow *ui;
    QSettings *settings;

    double sun_azimuth = 0;
    double sun_altitude = 0;

    QString ip = "192.168.0.176";
    unsigned short int port = 4805;
    QString station_id = "AGO";

    void display_cover_status(void);
    Station station;

    QString format_message(const QString& message) const;
};
#endif // MAINWINDOW_H
