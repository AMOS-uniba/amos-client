#include "include.h"
#include "ufomanager.h"

#include "windows.h"
#include "process.h"

extern EventLogger logger;

UfoManager::UfoManager(): m_autostart(false), m_state(UfoState::NOT_RUNNING) {}

UfoManager::~UfoManager(void) {
    this->stop_ufo();
}

UfoState UfoManager::state() const { return this->m_state; }

QString UfoManager::state_string() const {
    switch (this->m_state) {
        case UfoState::NOT_FOUND:
            return "file not found";
            break;
        case UfoState::NOT_EXE:
            return "file not an executable";
            break;
        case UfoState::NOT_RUNNING:
            return "not running";
            break;
        case UfoState::RUNNING:
            return QString("running (%1)").arg(this->m_process.pid()->dwProcessId);
            break;
        default:
            return "undefined";
            break;
    }
}

void UfoManager::update_state(QProcess::ProcessState state) {
    switch (state) {
        case QProcess::Running: {
            this->m_state = UfoState::RUNNING;
            break;
        }
        case QProcess::NotRunning:
        case QProcess::Starting: {
            this->m_state = UfoState::NOT_RUNNING;
            break;
        }
    }
}

bool UfoManager::is_running() {
    return (this->m_process.state() == QProcess::ProcessState::Running);

    if (this->m_frame == nullptr) {
        if (this->m_state == UfoState::RUNNING) {
            emit this->ufo_killed();
        }
        this->m_state = UfoState::NOT_RUNNING;
        return false;
    } else {
        if (this->m_state != UfoState::RUNNING) {
            emit this->ufo_started();
        }
        this->m_state = UfoState::RUNNING;
        return true;
    }
}

void UfoManager::auto_action(bool is_dark) {
    logger.debug("[UFO] Automatic action");

    if (this->m_autostart) {
        if (/* is_dark */ true) {
            this->start_ufo();
        } else {
            this->stop_ufo();
        }
    }
}

void UfoManager::start_ufo(void) {
    switch (this->m_process.state()) {
        case QProcess::ProcessState::Running:
        case QProcess::ProcessState::Starting: {
            logger.debug("[UFO] Already running");
            break;
        }
        case QProcess::ProcessState::NotRunning: {
            logger.info("[UFO] Starting");
            this->connect(&this->m_process, &QProcess::stateChanged, this, &UfoManager::update_state);
            this->m_process.start(this->m_path, {}, QIODevice::ReadWrite);
            break;
        }
    }
}

void UfoManager::stop_ufo(void) {
    /*
    HWND child;

    while (this->is_running()) {
        SendNotifyMessage(this->m_frame, WM_SYSCOMMAND, SC_CLOSE, 0);
        Sleep(500);
        while ((child = GetLastActivePopup(this->m_frame)) != nullptr) {
            SetActiveWindow(child);
            SendDlgItemMessage(child, 1, BM_CLICK, 0, 0);
        }
        emit this->ufo_killed();
    }*/

    if (this->m_process.state() == QProcess::ProcessState::Running) {
        this->m_process.terminate();
    }
}

qint64 UfoManager::pid(void) const {
    return this->m_pid;
}

const QString& UfoManager::path(void) const { return this->m_path; }

void UfoManager::set_path(const QString &path) {
    this->m_path = path;

    if (!QFile::exists(path)) {
        this->m_state = UfoState::NOT_FOUND;
    }

    if (!path.endsWith(".exe")) {
        this->m_state = UfoState::NOT_EXE;
    }
}

bool UfoManager::autostart(void) const { return this->m_autostart; }

void UfoManager::set_autostart(bool enable) {
    this->m_autostart = enable;
}

