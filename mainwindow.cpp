#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "include.h"

#include <random>
#include <chrono>

#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageLogger>


extern Log logger;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    this->ui->setupUi(this);

    this->universe = new Universe();

    this->ui->tb_log->setColumnWidth(0, 140);
    this->ui->tb_log->setColumnWidth(1, 80);

    this->settings = new QSettings("settings.ini", QSettings::IniFormat, this);
    this->settings->setValue("run/last_run", QDateTime::currentDateTimeUtc());
    this->load_settings();


    this->create_timers();
    this->display_cover_status();
    this->display_gizmo_status();
    this->display_station_config();
    this->display_storage_status();

    logger.set_display_widget(this->ui->tb_log);
    logger.info("Client initialized");

    // connect signals for handling of edits of station position
    connect(this->ui->dsb_latitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_position_edited);
    connect(this->ui->dsb_longitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_position_edited);
    connect(this->ui->dsb_altitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_position_edited);

    // connect signals for handling of edits of server address
    connect(this->ui->le_ip, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &MainWindow::station_position_edited);
    connect(this->ui->sb_port, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::station_position_edited);

    connect(this->ui->bt_fan, &QPushButton::clicked, this->station->dome_manager, &Dome::toggle_fan);
    connect(this->ui->bt_heating, &QPushButton::clicked, this->station->dome_manager, &Dome::toggle_heating);
    connect(this->ui->bt_intensifier, &QPushButton::clicked, this->station->dome_manager, &Dome::toggle_intensifier);
}

void MainWindow::load_settings(void) {
    QString ip = this->settings->value("server/ip").toString();
    unsigned short port = this->settings->value("server/port").toInt();
    QString station_id = this->settings->value("station/id").toString();

    this->server = new Server(this, QHostAddress(ip), port, station_id);

    this->station = new Station(
        station_id,
        QDir(this->settings->value("storage/primary").toString()),
        QDir(this->settings->value("storage/permanent").toString())
    );

    this->station->set_position(
        this->settings->value("station/latitude", 0).toDouble(),
        this->settings->value("station/longitude", 0).toDouble(),
        this->settings->value("station/altitude", 0).toDouble()
    );
    this->station->set_altitude_dark(this->settings->value("station/dark", -7.0).toDouble());
}

void MainWindow::create_timers(void) {
    this->request_telegram();
    this->timer_telegram = new QTimer(this);
    this->timer_telegram->setInterval(3000);
    connect(timer_telegram, &QTimer::timeout, this, &MainWindow::request_telegram);
    this->timer_telegram->start();

    this->timer_heartbeat = new QTimer(this);
    this->timer_heartbeat->setInterval(15 * 1000);
    connect(this->timer_heartbeat, &QTimer::timeout, this, &MainWindow::send_heartbeat);
    this->timer_heartbeat->start();

    this->timer_operation = new QTimer(this);
    this->timer_operation->setInterval(1000);
    connect(this->timer_operation, &QTimer::timeout, this, &MainWindow::process_timer);
    this->timer_operation->start();

    this->timer_cover = new QTimer(this);
    this->timer_cover->setInterval(10);
    connect(this->timer_cover, &QTimer::timeout, this, &MainWindow::move_cover);}

MainWindow::~MainWindow() {
    logger.info("Terminating");

    delete this->ui;
    delete this->timer_operation;
    delete this->timer_cover;
    delete this->timer_heartbeat;
    delete this->timer_telegram;

    delete this->universe;
    delete this->station;
    delete this->server;
}

void MainWindow::on_actionExit_triggered() {
    QApplication::quit();
}

void MainWindow::process_timer(void) {
    this->station->check_sun();
    this->display_time();
    this->display_env_data();
    this->display_sun_properties();
}

void MainWindow::display_time(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    ui->label_heartbeat->setText(QString("%1 s").arg((double) this->timer_heartbeat->remainingTime() / 1000, 3, 'f', 1));
}

void MainWindow::display_sun_properties(void) {
    auto hor = this->station->sun_position();
    ui->lb_hor_altitude->setText(QString("%1°").arg(hor.theta * Deg, 3, 'f', 3));
    ui->lb_hor_azimuth->setText(QString("%1°").arg(fmod(hor.phi * Deg + 360.0, 360.0), 3, 'f', 3));

    auto equ = Universe::compute_sun_equ();
    ui->lb_eq_latitude->setText(QString("%1°").arg(equ[theta] * Deg, 3, 'f', 3));
    ui->lb_eq_longitude->setText(QString("%1°").arg(equ[phi] * Deg, 3, 'f', 3));

    auto ecl = Universe::compute_sun_ecl();
    ui->lb_ecl_longitude->setText(QString("%1°").arg(ecl[phi] * Deg, 3, 'f', 3));
}

void MainWindow::display_env_data(void) {
    ui->group_environment->setTitle(QString("Environment (%1 s)").arg((double) this->station->dome_manager->get_last_received().msecsTo(QDateTime::currentDateTimeUtc()) / 1000, 3, 'f', 1));
    ui->label_temp->setText(QString("%1 °C").arg(this->station->dome_manager->temperature, 3, 'f', 1));
    ui->label_press->setText(QString("%1 kPa").arg(this->station->dome_manager->pressure / 1000, 5, 'f', 3));
    ui->label_hum->setText(QString("%1 %").arg(this->station->dome_manager->humidity, 3, 'f', 1));
}

void MainWindow::display_storage_status(const Storage& storage, QProgressBar *pb, QLineEdit *le) {
    QStorageInfo info = storage.info();
    unsigned int total = (int) ((double) info.bytesTotal() / (1 << 30));
    unsigned int used = (int) ((double) info.bytesAvailable() / (1 << 30));
    pb->setMaximum(total);
    pb->setValue(total - used);
    le->setText(storage.get_directory().path());
}

void MainWindow::display_storage_status(void) {
    this->display_storage_status(this->station->get_primary_storage(), this->ui->pb_primary, this->ui->le_primary);
    this->display_storage_status(this->station->get_permanent_storage(), this->ui->pb_permanent, this->ui->le_permanent);
}

void MainWindow::display_station_config(void) {
    this->ui->le_ip->setText(this->server->get_address().toString());
    this->ui->sb_port->setValue(this->server->get_port());
    this->ui->le_station_id->setText(this->station->get_id());

    this->ui->dsb_latitude->setValue(this->station->latitude);
    this->ui->dsb_longitude->setValue(this->station->longitude);
    this->ui->dsb_altitude->setValue(this->station->altitude);
}

void MainWindow::send_heartbeat(void) {
    this->server->send_heartbeat(this->station->prepare_heartbeat());
}

void MainWindow::move_cover(void) {
    switch (this->station->dome_manager->cover_state) {
        case CoverState::OPENING: {
            if (this->station->dome_manager->cover_position < 400) {
                this->station->dome_manager->cover_position += 1;
            } else {
                this->station->dome_manager->cover_state = CoverState::OPEN;
                ui->button_cover->setEnabled(true);
                this->timer_cover->stop();
            }
            break;
        }
        case CoverState::CLOSING: {
            if (this->station->dome_manager->cover_position > 0) {
                this->station->dome_manager->cover_position -= 1;
            } else {
                this->station->dome_manager->cover_state = CoverState::CLOSED;
                ui->button_cover->setEnabled(true);
                this->timer_cover->stop();
            }
            break;
        }
        default:
            break;
    }
    this->display_cover_status();
}

void MainWindow::request_telegram(void) {
    /* Currently a mockup */
    this->station->dome_manager->fake_env_data();
}

void MainWindow::display_cover_status(void) {
    QString caption;
    switch (this->station->dome_manager->cover_state) {
        case CoverState::OPEN: {
            caption = "Open";
            ui->button_cover->setText("Close");
            break;
        }
        case CoverState::OPENING: {
            caption = "Opening...";
            break;
        }
        case CoverState::CLOSED: {
            caption = "Closed";
            ui->button_cover->setText("Open");
            break;
        }
        case CoverState::CLOSING: {
            caption = "Closing...";
            break;
        }
        default: {
            caption = "undefined";
            break;
        }
    }
    ui->progress_cover->setValue(this->station->dome_manager->cover_position);
    ui->label_cover_state->setText(caption);
}

void MainWindow::display_gizmo_status(void) {
    this->ui->lb_fan->setText(Dome::Ternary[this->station->dome_manager->fan_state].display_name);
    this->ui->bt_fan->setText(this->station->dome_manager->fan_state == TernaryState::ON ? "disable" : "enable");

    this->ui->lb_intensifier->setText(Dome::Ternary[this->station->dome_manager->intensifier_state].display_name);
    this->ui->bt_intensifier->setText(this->station->dome_manager->intensifier_state == TernaryState::ON ? "disable" : "enable");
}

void MainWindow::on_button_send_heartbeat_pressed() {
    this->send_heartbeat();
}

void MainWindow::on_button_cover_clicked() {
    if (this->station->dome_manager->cover_state == CoverState::CLOSED) {
        this->station->dome_manager->open_cover();
    }
    if (this->station->dome_manager->cover_state == CoverState::OPEN) {
        this->station->dome_manager->open_cover();
    }
    this->display_cover_status();
    this->timer_cover->start();
    ui->button_cover->setEnabled(false);
}

void MainWindow::on_bt_station_apply_clicked() {
    QString address = ui->le_ip->text();
    unsigned short port = ui->sb_port->value();

    this->station->set_id(this->ui->le_station_id->text());
    this->server->set_url(QHostAddress(address), port, this->station->get_id());
    this->station->set_position(ui->dsb_latitude->value(), ui->dsb_longitude->value(), this->ui->dsb_altitude->value());

    this->settings->setValue("server/ip", this->server->get_address().toString());
    this->settings->setValue("server/port", this->server->get_port());
    this->settings->setValue("station/id", this->station->get_id());
    this->settings->setValue("station/latitude", this->station->latitude);
    this->settings->setValue("station/longitude", this->station->longitude);
    this->settings->setValue("station/altitude", this->station->altitude);

    this->settings->sync();

    this->button_station_toggle(false);
}

void MainWindow::on_bt_station_reset_clicked() {
    this->ui->le_station_id->setText(this->station->get_id());
    this->ui->le_ip->setText(this->server->get_address().toString());
    this->ui->sb_port->setValue(this->server->get_port());

    this->ui->dsb_latitude->setValue(this->station->latitude);
    this->ui->dsb_longitude->setValue(this->station->longitude);
    this->ui->dsb_altitude->setValue(this->station->altitude);
}

void MainWindow::on_checkbox_manual_stateChanged(int enable) {
    if (enable) {
        this->setWindowTitle("AMOS client [manual mode]");
        logger.warning("Switched to manual mode");
    } else {
        this->setWindowTitle("AMOS client [automatic mode]");
        logger.warning("Switched to automatic mode");
    }
    this->station->automatic = (bool) enable;
    ui->group_control->setEnabled(this->station->automatic);
}

void MainWindow::station_position_edited(void) {
    this->button_station_toggle(
        (ui->le_station_id->text() != this->station->get_id()) ||
        (ui->dsb_latitude->value() != this->station->latitude) ||
        (ui->dsb_longitude->value() != this->station->longitude) ||
        (ui->dsb_altitude->value() != this->station->altitude) ||
        (ui->le_ip->text() != this->server->get_address().toString()) ||
        (ui->sb_port->value() != this->server->get_port())
    );
}

void MainWindow::button_station_toggle(bool enable) {
    if (enable) {
        this->ui->bt_station_apply->setText("Apply changes");
        this->ui->bt_station_apply->setEnabled(true);
    } else {
        this->ui->bt_station_apply->setText("No changes");
        this->ui->bt_station_apply->setEnabled(false);
    }
}

void MainWindow::set_storage(Storage& storage, QLineEdit& edit) {
    QString new_dir = QFileDialog::getExistingDirectory(
        this,
        QString("Select %1 storage directory").arg(storage.get_name()),
        storage.get_directory().path(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (new_dir == "") {
        logger.debug("Directory selection aborted");
        return;
    } else {
        storage.set_directory(new_dir);
        edit.setText(new_dir);
        this->display_storage_status();
        this->settings->setValue(QString("storage/%1").arg(storage.get_name()), new_dir);
    }
}

void MainWindow::on_bt_primary_clicked() {
    this->set_storage(this->station->get_primary_storage(), *this->ui->le_primary);
}

void MainWindow::on_bt_permanent_clicked() {
    this->set_storage(this->station->get_permanent_storage(), *this->ui->le_permanent);
}

void MainWindow::on_pushButton_clicked() {
    try {
        Telegram telegram(QByteArray(ui->le_telegram->text().toUtf8()));
    } catch (RuntimeException& e) {
        logger.error(e.what());
    }
}

void MainWindow::on_pushButton_2_clicked() {
    try {
        Telegram telegram = Telegram(0x55, ui->le_telegram->text().toUtf8());
        logger.info("Encoding...");
        QByteArray ba = telegram.compose();
        logger.info(QString("Encoded to %1").arg(QString(ba)));
        Telegram orig = Telegram(ba);
        logger.info(QByteArray(orig.compose()));
    } catch (RuntimeException& e) {
        logger.error(QString("Telegram is malformed: %1").arg(e.what()));
    }
}
