#include <QFileDialog>

#include "qufomanager.h"
#include "ui_qufomanager.h"
#include "logging/eventlogger.h"

extern EventLogger logger;
extern QSettings * settings;


QUfoManager::QUfoManager(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QUfoManager),
    m_autostart(false),
    m_state(UfoState::NotRunning)
{
    ui->setupUi(this);

    this->m_timer_check = new QTimer(this);
    this->m_timer_check->setInterval(1000);
    this->connect(this->m_timer_check, &QTimer::timeout, this, &QUfoManager::update_state);
    this->m_timer_check->start();
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

    switch (this->m_process.state()) {
        case QProcess::ProcessState::Running: {
            this->ui->lb_state->setText(QString("running (%1)").arg(this->m_process.processId()));
            this->ui->lb_state->setStyleSheet("QLabel { color: green; }");
            this->ui->bt_toggle->setEnabled(true);
            this->ui->bt_toggle->setText("Stop UFO");
            this->connect(this->ui->bt_toggle, &QPushButton::clicked, this, &QUfoManager::stop_ufo);
            break;
        }
        case QProcess::ProcessState::Starting: {
            this->ui->lb_state->setText("starting");
            this->ui->lb_state->setStyleSheet("QLabel { color: orange; }");
            this->ui->bt_toggle->setEnabled(false);
            this->ui->bt_toggle->setText("Starting");
            break;
        }
        case QProcess::ProcessState::NotRunning: {
            QFileInfo info(this->path());
            if (info.exists()) {
                if (this->path().endsWith(".exe") && info.isFile()) {
                    this->ui->lb_state->setText("not running");
                    this->ui->lb_state->setStyleSheet("QLabel { color: gray; }");
                    this->ui->bt_toggle->setEnabled(true);
                    this->ui->bt_toggle->setText("Run UFO");
                    this->connect(this->ui->bt_toggle, &QPushButton::clicked, this, &QUfoManager::start_ufo);
                } else {
                    this->ui->lb_state->setText("not an exe file");
                    this->ui->lb_state->setStyleSheet("QLabel { color: red; }");
                    this->ui->bt_toggle->setEnabled(false);
                    this->ui->bt_toggle->setText("Error");
                    break;
                }
            } else {
                this->ui->lb_state->setText("not found");
                this->ui->lb_state->setStyleSheet("QLabel { color: red; }");
                this->ui->bt_toggle->setEnabled(false);
                this->ui->bt_toggle->setText("Error");
                break;
            }
        }
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
        int attempt = 3;
        while (this->is_running() && attempt-- > 0) {
            logger.info(Concern::UFO, QString("Trying to stop UFO politely (attempt %1)").arg(3 - attempt));
            SendNotifyMessage(this->m_frame, WM_SYSCOMMAND, SC_CLOSE, 0);
            Sleep(200);
            while ((child = GetLastActivePopup(this->m_frame)) != nullptr) {
                SetActiveWindow(child);
                SendDlgItemMessage(child, 1, BM_CLICK, 0, 0);
            }
            emit this->stopped();
            return;
        }

        logger.warning(Concern::UFO, "Killing the child process");
        this->m_process.kill();
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
