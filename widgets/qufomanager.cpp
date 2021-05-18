#include <QFileDialog>

#include "qufomanager.h"
#include "ui_qufomanager.h"
#include "logging/eventlogger.h"

extern EventLogger logger;
extern QSettings * settings;

const UfoState QUfoManager::Unknown      = UfoState('U', "unknown", Qt::black, false, "Error");
const UfoState QUfoManager::NotAnExe     = UfoState('E', "not an EXE file", Qt::red, false, "Error");
const UfoState QUfoManager::NotFound     = UfoState('F', "not found", Qt::red, false, "Error");
const UfoState QUfoManager::NotRunning   = UfoState('N', "not running", Qt::gray, true, "Start UFO");
const UfoState QUfoManager::Starting     = UfoState('S', "starting", QColor::fromHsl(60, 255, 255), false, "Starting...");
const UfoState QUfoManager::Running      = UfoState('R', "running", Qt::darkGreen, true, "Stop UFO");

QUfoManager::QUfoManager(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QUfoManager),
    m_frame(nullptr),
    m_autostart(false),
    m_state(QUfoManager::NotRunning)
{
    ui->setupUi(this);

    this->m_timer_check = new QTimer(this);
    this->m_timer_check->setInterval(1000);
    this->connect(this->m_timer_check, &QTimer::timeout, this, &QUfoManager::update_state);
    this->m_timer_check->start();

    this->connect(this, &QUfoManager::state_changed, this, &QUfoManager::log_state_change);
}

QUfoManager::~QUfoManager() {
    delete ui;
    delete this->m_timer_check;
}

void QUfoManager::initialize(void) {
    this->load_settings();
}

void QUfoManager::load_settings(void) {
    this->set_path(settings->value("ufo/path", "C:\\Program Files\\UFO\\UFO.exe").toString());
    this->set_autostart(settings->value("ufo/autostart", false).toBool());
}

void QUfoManager::save_settings(void) const {
    settings->setValue("ufo/path", this->path());
    settings->setValue("ufo/autostart", this->is_autostart());
}

// Autostart getter and setter
void QUfoManager::set_autostart(bool enable) {
    logger.info(Concern::UFO, QString("Autostart %1abled").arg(enable ? "en" : "dis"));
    this->m_autostart = enable;

    this->ui->cb_auto->setCheckState(enable ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
}

bool QUfoManager::is_autostart(void) const { return this->m_autostart; }

void QUfoManager::set_path(const QString & path) {
    this->ui->le_path->setText(path);
    this->m_path = path;
    this->update_state();
}

const QString& QUfoManager::path(void) const { return this->m_path; }

void QUfoManager::auto_action(bool is_dark) const {
    if (this->m_autostart) {
        logger.debug(Concern::UFO, "Automatic action");
        if (is_dark) {
            this->start_ufo();
        } else {
            this->stop_ufo();
        }
    }
}

void QUfoManager::update_state(void) {
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

    if (old_state != new_state) {
        this->m_state = new_state;
        emit this->state_changed(new_state);
    }
}

bool QUfoManager::is_running(void) const {
    return (this->m_process.state() == QProcess::ProcessState::Running);
}

/**
 * @brief QUfoManager::start_ufo
 * Conditionally start UFO Capture v2 as a child process
 */
void QUfoManager::start_ufo(void) const {
    switch (this->m_process.state()) {
        case QProcess::ProcessState::Running:
        case QProcess::ProcessState::Starting: {
            logger.debug(Concern::UFO, "Already running");
            break;
        }
        case QProcess::ProcessState::NotRunning: {
            logger.info(Concern::UFO, "Starting");
            this->m_process.setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedChannels);
            this->m_process.setWorkingDirectory(QFileInfo(this->m_path).absoluteDir().path());
            this->connect(&this->m_process, &QProcess::stateChanged, this, &QUfoManager::update_state);
            this->m_process.start(this->m_path, {}, QProcess::OpenMode(QProcess::ReadWrite));

            Sleep(1000);
            this->m_frame = FindWindowA(nullptr, "UFOCapture");
            logger.debug(Concern::UFO, QString("HWND is %1").arg((long long) this->m_frame));
            Sleep(1000);
            ShowWindowAsync(this->m_frame, SW_SHOWMINIMIZED);
            emit this->started();
            break;
        }
    }
}

/**
 * @brief QUfoManager::stop_ufo
 * Stops UFO Capture v2 (three polite attempts by Jozef's method, then kill)
 */
void QUfoManager::stop_ufo(void) const {
    if (this->m_process.state() == QProcess::ProcessState::NotRunning) {
        logger.debug(Concern::UFO, "Not running");
    } else {
        HWND child;

        if (this->is_running()) {
            logger.debug(Concern::UFO, QString("Trying to stop UFO politely"));
            SendNotifyMessage(this->m_frame, WM_SYSCOMMAND, SC_CLOSE, 0);
            Sleep(1000);

            logger.debug(Concern::UFO, "Clicking the dialog button");
            child = GetLastActivePopup(this->m_frame);

            logger.debug(Concern::UFO, QString("Child dialog's HWND is %1").arg((long long) child));

            if (child == nullptr) {
                logger.error(Concern::UFO, "Child is null");
            }

            SetActiveWindow(child);
            SendDlgItemMessage(child, 1, BM_CLICK, 0, 0);

            Sleep(1000);

            if (this->is_running()) {
                logger.warning(Concern::UFO, "UFO did not stop, killing the child process");
                // this->m_process.kill();
            }

            emit this->stopped();
        } else {
            logger.debug(Concern::UFO, "UFO is not running, not doing anything");
        }
    }
}

void QUfoManager::log_state_change(const UfoState & state) const {
    QString message = QString("State changed to \"%1\"").arg(state.display_string());

    if (state == QUfoManager::Starting) {
        logger.debug(Concern::UFO, message);
    } else {
        if ((state == QUfoManager::NotRunning) || (state == QUfoManager::Running)) {
            logger.info(Concern::UFO, message);
        } else {
            logger.error(Concern::UFO, message);
        }
    }

}

void QUfoManager::on_cb_auto_clicked(bool checked) {
    this->set_autostart(checked);
    settings->setValue("ufo/autostart", this->is_autostart());
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
            settings->setValue("ufo/path", filename);
            this->set_path(filename);
        }
    }
}
