#include <QFileDialog>

#include "qufomanager.h"
#include "ui_qufomanager.h"
#include "logging/eventlogger.h"

extern EventLogger logger;
extern QSettings * settings;


QUfoManager::QUfoManager(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QUfoManager),
    m_pid(-1),
    m_state(UfoState::NotRunning)
{
    ui->setupUi(this);
}

QUfoManager::~QUfoManager() {
    delete ui;
}

void QUfoManager::initialize(const QStation * const station) {
    this->m_station = station;
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
void QUfoManager::set_autostart(bool enable) { this->m_autostart = enable; }
bool QUfoManager::is_autostart(void) const { return this->m_autostart; }

void QUfoManager::set_path(const QString & path) {
    this->ui->le_path->setText(path);
    this->m_path = path;

    if (!QFile::exists(path)) {
        this->update_state(UfoState::NotFound);
    }

    if (!path.endsWith(".exe")) {
        this->update_state(UfoState::NotExe);
    }
}

const QString& QUfoManager::path(void) const { return this->m_path; }

void QUfoManager::auto_action(bool is_dark) {
    logger.debug(Concern::UFO, "Automatic action");
    if (this->m_autostart) {
        if (is_dark) {
            this->start_ufo();
        } else {
            this->stop_ufo();
        }
    }
}

void QUfoManager::update_state(const UfoState new_state) {
    this->disconnect(this->ui->bt_toggle, &QPushButton::clicked, nullptr, nullptr);

    switch (new_state) {
        case UfoState::NotRunning: {
            this->ui->bt_toggle->setEnabled(true);
            this->ui->bt_toggle->setText("Run UFO");
            this->connect(this->ui->bt_toggle, &QPushButton::clicked, this, &QUfoManager::start_ufo);
            break;
        }
        case UfoState::NotExe: {
            this->ui->bt_toggle->setEnabled(false);
            this->ui->bt_toggle->setText("Not an exe file");
            break;
        }
        case UfoState::NotFound: {
            this->ui->bt_toggle->setEnabled(false);
            this->ui->bt_toggle->setText("Not found");
            break;
        }
        case UfoState::Starting: {
            this->ui->bt_toggle->setEnabled(false);
            this->ui->bt_toggle->setText("Starting");
            break;
        }
        case UfoState::Running: {
            this->ui->bt_toggle->setEnabled(true);
            this->ui->bt_toggle->setText("Stop UFO");
            this->connect(this->ui->bt_toggle, &QPushButton::clicked, this, &QUfoManager::stop_ufo);
            break;
        }
    }

    this->m_state = new_state;
}

bool QUfoManager::is_running(void) const {
    return (this->m_process.state() == QProcess::ProcessState::Running);
}

void QUfoManager::start_ufo(void) {
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
            //this->connect(&this->m_process, &QProcess::stateChanged, this, &QUfoManager::update_state);
            this->m_process.start(this->m_path, {}, QProcess::OpenMode(QProcess::ReadWrite));
            emit this->started();
            break;
        }
    }
}

void QUfoManager::stop_ufo(void) {
    HWND child;
    while (this->is_running()) {
        SendNotifyMessage(this->m_frame, WM_SYSCOMMAND, SC_CLOSE, 0);
        Sleep(500);
        while ((child = GetLastActivePopup(this->m_frame)) != nullptr) {
            SetActiveWindow(child);
            SendDlgItemMessage(child, 1, BM_CLICK, 0, 0);
        }
        emit this->stopped();
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
            logger.debug(Concern::UFO, "Path changed");
            settings->setValue("ufo/path", filename);
            this->set_path(filename);
        }
    }
}
