#include <QFileDialog>
#include <QTimer>
#include <QJsonObject>

#include "qufomanager.h"
#include "ui_qufomanager.h"
#include "logging/eventlogger.h"
#include "utils/exceptions.h"


extern EventLogger logger;
extern QSettings * settings;

const UfoState QUfoManager::Unknown      = UfoState('U', "unknown", Qt::black, false, "Error");
const UfoState QUfoManager::NotAnExe     = UfoState('E', "not an EXE file", Qt::red, false, "Error");
const UfoState QUfoManager::NotFound     = UfoState('F', "not found", Qt::red, false, "Error");
const UfoState QUfoManager::NotRunning   = UfoState('N', "not running", Qt::gray, true, "Start UFO");
const UfoState QUfoManager::Starting     = UfoState('S', "starting", QColor::fromHsl(60, 255, 255), false, "Starting...");
const UfoState QUfoManager::Running      = UfoState('R', "running", Qt::darkGreen, true, "Stop UFO");

const QString QUfoManager::DefaultPathAllSky = "C:/AMOS/UFO2/UFO2.exe";
const QString QUfoManager::DefaultPathSpectral = "C:/AMOS/UFOHD2/UFOHD2.exe";

QUfoManager::QUfoManager(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QUfoManager),
    m_start_scheduled(false),
    m_frame(nullptr),
    m_path(""),
    m_id(""),
    m_autostart(false),
    m_state(QUfoManager::NotRunning)
{
    ui->setupUi(this);

    this->m_timer_check = new QTimer(this);
    this->m_timer_check->setInterval(1000);
    this->connect(this->m_timer_check, &QTimer::timeout, this, &QUfoManager::update_state);
    this->m_timer_check->start();

    this->m_timer_delay = new QTimer(this);

    this->connect(this, &QUfoManager::state_changed, this, &QUfoManager::log_state_change);
}

QUfoManager::~QUfoManager() {
    this->disconnect(&this->m_process, &QProcess::stateChanged, nullptr, nullptr);
    delete this->m_timer_check;
    delete this->m_timer_delay;
    delete this->ui;
}

void QUfoManager::initialize(const QString & id) {
    if (!this->m_id.isEmpty()) {
        throw ConfigurationError("UFO manager id already set");
    }
    this->m_id = id;

    this->load_settings();
    this->update_state();
}

void QUfoManager::load_settings(void) {
    this->set_path(settings->value(QString("camera_%1/ufo_path").arg(this->id()), QUfoManager::DefaultPathAllSky).toString());
    this->set_autostart(settings->value(QString("camera_%1/ufo_autostart").arg(this->id()), QUfoManager::DefaultEnabled).toBool());
}

void QUfoManager::save_settings(void) const {
    settings->setValue(QString("camera_%1/ufo_path").arg(this->id()), this->m_path);
    settings->setValue(QString("camera_%1/ufo_autostart").arg(this->id()), this->is_autostart());
}

// Autostart getter and setter
void QUfoManager::set_autostart(bool enable) {
    logger.info(Concern::UFO, QString("UFO-%1: autostart %2abled").arg(this->id(), enable ? "en" : "dis"));
    this->m_autostart = enable;
    this->m_start_scheduled &= enable;

    this->ui->cb_auto->setCheckState(enable ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
}

// Path getter and setter
void QUfoManager::set_path(const QString & path) {
    logger.info(Concern::UFO, QString("UFO-%1: path set to \"%2\"").arg(this->id(), path));
    this->ui->le_path->setText(path);
    this->m_path = path;
}

// Automatic action: start UFO after sunset, stop before sunrise
void QUfoManager::auto_action(bool is_dark, const QDateTime & open_since) {
    if (this->m_autostart) {
        logger.debug(Concern::UFO, QString("UFO-%1: Automatic action").arg(this->id()));

        if (this->state() == QUfoManager::NotAnExe) {
            logger.debug_error(Concern::UFO, QString("UFO-%1: Selected file is not a valid EXE file").arg(this->id()));
        } else {
            if (this->state() == QUfoManager::NotFound) {
                logger.debug_error(Concern::UFO, QString("UFO-%1: File not found").arg(this->id()));
            } else {
                if (is_dark) {
                    if (!open_since.isValid()) {
                        logger.debug(Concern::UFO, QString("Camera %1: Dome is not open or II is off").arg(this->id()));
                    } else {
                        logger.debug(Concern::UFO, QString("Camera %1: Cover open and II active for %2 s")
                            .arg(this->id())
                            .arg(open_since.secsTo(QDateTime::currentDateTimeUtc()))
                        );
                        if (open_since.secsTo(QDateTime::currentDateTimeUtc()) > 10) {
                            this->start_ufo();
                        }
                    }
                } else {
                    this->stop_ufo();
                }
            }
        }
    }
}

void QUfoManager::update_state(void) {
    logger.debug(Concern::UFO, QString("UFO-%1: Updating state...").arg(this->id()));

    this->disconnect(this->ui->bt_toggle, &QPushButton::clicked, nullptr, nullptr);

    UfoState old_state = this->m_state;
    UfoState new_state = QUfoManager::Unknown;

    switch (this->m_process.state()) {
        case QProcess::ProcessState::Running: {
            this->connect(this->ui->bt_toggle, &QPushButton::clicked, this, &QUfoManager::stop_ufo);
            new_state = QUfoManager::Running;
            break;
        }
        case QProcess::ProcessState::Starting: {
            new_state = QUfoManager::Starting;
            break;
        }
        case QProcess::ProcessState::NotRunning: {
            QFileInfo info(this->path());
            if (info.exists()) {
                if (this->path().endsWith(".exe") && info.isFile()) {
                    this->connect(this->ui->bt_toggle, &QPushButton::clicked, this, &QUfoManager::start_ufo);
                    new_state = QUfoManager::NotRunning;
                } else {
                    new_state = QUfoManager::NotAnExe;
                    break;
                }
            } else {
                new_state = QUfoManager::NotFound;
                break;
            }
        }
    }

    this->ui->lb_state->setText(new_state.display_string());
    this->ui->lb_state->setStyleSheet(QString("QLabel { color: %1; }").arg(new_state.colour().name()));
    this->ui->bt_toggle->setEnabled(new_state.button_enabled());
    this->ui->bt_toggle->setText(new_state.button_text());
    this->ui->cb_auto->setEnabled(new_state.button_enabled());

    if (old_state != new_state) {
        this->m_state = new_state;
        emit this->state_changed(new_state);
    }
}

/**
 * @brief QUfoManager::start_ufo
 * Conditionally start UFO Capture v2 as a child process
 */
void QUfoManager::start_ufo(unsigned int delay) const {
    switch (this->m_process.state()) {
        case QProcess::ProcessState::Running:
        case QProcess::ProcessState::Starting: {
            logger.debug(Concern::UFO, QString("UFO-%1 already running, not doing anything").arg(this->id()));
            break;
        }
        case QProcess::ProcessState::NotRunning: {
            if (this->m_start_scheduled) {
                logger.debug(Concern::UFO, QString("UFO-%1 already scheduled to start").arg(this->id()));
            } else {
                logger.info(Concern::UFO, QString("UFO-%1 starting with delay %2 s").arg(this->id()).arg(delay));

                this->m_start_scheduled = true;
                this->m_timer_delay->setInterval(delay * 1000);
                this->connect(this->m_timer_delay, &QTimer::timeout, this, &QUfoManager::start_ufo_inner);
                this->m_timer_delay->setSingleShot(true);
                this->m_timer_delay->start();
                break;
            }
        }
    }
}

/**
 * @brief QUfoManager::start_ufo_inner
 * Actually starts UFO, private function
 */
void QUfoManager::start_ufo_inner(void) {
    logger.debug(Concern::UFO, QString("UFO-%1 starting").arg(this->id()));
    this->m_process.setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedChannels);
    this->m_process.setWorkingDirectory(QFileInfo(this->m_path).absoluteDir().path());
    this->connect(&this->m_process, &QProcess::stateChanged, this, &QUfoManager::update_state);
    this->m_process.start(this->m_path, {}, QProcess::OpenMode(QProcess::ReadWrite));

    Sleep(1000);
    this->m_frame = FindWindowA(nullptr, "UFOCapture");
    logger.debug(Concern::UFO, QString("UFO-%1 HWND is %2").arg(this->id()).arg((long long) this->m_frame));
    Sleep(1000);
    ShowWindowAsync(this->m_frame, SW_SHOWMINIMIZED);
    this->m_start_scheduled = false;

    emit this->started();
}

/**
 * @brief QUfoManager::stop_ufo
 * Stops UFO Capture v2 (three polite attempts by Jozef's method, then kill)
 */
void QUfoManager::stop_ufo(void) {
    if (this->m_process.state() == QProcess::ProcessState::NotRunning) {
        logger.debug(Concern::UFO, QString("UFO-%1: Not running").arg(this->id()));
    } else {
        HWND child;

        if (this->is_running()) {
            logger.info(Concern::UFO, QString("UFO-%1 stopping").arg(this->id()));
            SendNotifyMessage(this->m_frame, WM_SYSCOMMAND, SC_CLOSE, 0);
            Sleep(200);

            logger.debug(Concern::UFO, "Clicking the dialog button");
            child = GetLastActivePopup(this->m_frame);

            logger.debug(Concern::UFO, QString("Child dialog's HWND is %1").arg((long long) child));

            if (child == nullptr) {
                logger.debug_error(Concern::UFO, "Child is null");
            } else {
                SetActiveWindow(child);
                SendDlgItemMessage(child, 1, BM_CLICK, 0, 0);
                Sleep(200);
            }

            if (this->is_running()) {
                logger.warning(Concern::UFO, QString("UFO-%1: Application did not stop, killing the child process").arg(this->id()));
                this->m_process.kill();
            }

            emit this->stopped();
        } else {
            logger.debug(Concern::UFO, QString("UFO-%1: Application is not running, not doing anything").arg(this->id()));
        }
    }
}

void QUfoManager::log_state_change(const UfoState & state) const {
    logger.debug(Concern::UFO, QString("UFO-%1: State changed to \"%2\"").arg(this->id(), state.display_string()));
}

QJsonObject QUfoManager::json(void) const {
    return QJsonObject {
        {"auto", this->is_autostart()},
        {"st", QString(QChar(this->state().code()))},
    };
}

/** Event handlers **/

void QUfoManager::on_cb_auto_clicked(bool checked) {
    this->set_autostart(checked);
    this->save_settings();
}

void QUfoManager::on_bt_change_clicked(void) {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Select UFO executable",
        QString(),
        "Executable file (*.exe)"
    );

    if (filename == "") {
        logger.debug(Concern::UFO, "Directory selection aborted");
        return;
    } else {
        if (filename == this->path()) {
            logger.debug(Concern::UFO, "Path not changed");
        } else {
            logger.info(Concern::UFO, QString("Path changed to %1").arg(filename));
            this->set_path(filename);
            this->update_state();
            this->save_settings();
        }
    }
}
