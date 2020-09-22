#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <random>

#include <QMainWindow>
#include <QMap>
#include <QTimer>
#include <QSerialPort>
#include <QNetworkAccessManager>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


enum CoverState {
    COVER_OPEN,
    COVER_OPENING,
    COVER_CLOSING,
    COVER_CLOSED
};

enum Telegram {
    TELEGRAM_NOP,                       // do nothing, for testing
    TELEGRAM_COVER_OPEN,                // open the cover
    TELEGRAM_COVER_CLOSE,               // close the cover
    TELEGRAM_FAN_ON,                    // turn on the fan
    TELEGRAM_FAN_OFF,                   // turn off the fan
    TELEGRAM_II_ON,                     // turn on image intensifier
    TELEGRAM_II_OFF,                    // turn off image intensifier
    TELEGRAM_SW_RESET                   // perform software reset
};

static QMap<Telegram, QChar> Telegrams = {
    {TELEGRAM_NOP, '\x00'},
    {TELEGRAM_COVER_OPEN, '\x01'},
    {TELEGRAM_COVER_CLOSE, '\x02'},
    {TELEGRAM_FAN_ON, '\x05'},
    {TELEGRAM_FAN_OFF, '\x06'},
    {TELEGRAM_II_ON, '\x07'},
    {TELEGRAM_II_OFF, '\x08'},
    {TELEGRAM_SW_RESET, '\x0b'}
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    bool manual = false;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionExit_triggered();

    void on_cbManual_stateChanged(int arg1);

    void process_timer(void);
    void request_telegram(void);

    void display_housekeeping_data(void);
    void get_housekeeping_data(void);
    void send_heartbeat(void);

    void handleEnd(QNetworkReply *a);

    void on_button_send_heartbeat_pressed();

    void on_button_cover_clicked();
    void move_cover(void);

private:
    QTimer *timer_operation, *timer_cover, *timer_telegram;
    QNetworkAccessManager *network_manager;
    Ui::MainWindow *ui;

    std::default_random_engine generator;

    double temperature = 0;
    double pressure = 0;
    double humidity = 0;

    CoverState cover_state = COVER_CLOSED;
    unsigned int cover_position = 0;
    bool fan = false;
    bool heating = false;

    void display_cover_status(void);
    QDateTime last_received;
};
#endif // MAINWINDOW_H
