#include "include.h"
#include "ufomanager.h"

#include "windows.h"
#include "process.h"

extern EventLogger logger;

UfoManager::UfoManager(): m_state(UfoState::NOT_RUNNING) {}

UfoManager::~UfoManager(void) {

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
    this->m_frame = FindWindowA(nullptr, "UFOCaptureHD2");
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

void UfoManager::start_ufo(void) {
    switch (this->m_state) {
        case UfoState::RUNNING: {
            logger.info("Already running");
            break;
        }
        case UfoState::NOT_RUNNING: {
            logger.info("Starting UFO");
            this->connect(&this->m_process, &QProcess::stateChanged, this, &UfoManager::update_state);
            this->m_process.start(this->m_path, {}, QIODevice::ReadWrite);
            break;
        }
        default: {
            logger.info("Cannot run UFO");
            break;
        }
    }
}

void UfoManager::kill_ufo(void) {
    HWND child;

    while (this->is_running()) {
        SendNotifyMessage(this->m_frame, WM_SYSCOMMAND, SC_CLOSE, 0);
        Sleep(500);
        while ((child = GetLastActivePopup(this->m_frame)) != nullptr) {
            SetActiveWindow(child);
            SendDlgItemMessage(child, 1, BM_CLICK, 0, 0);
        }
        emit this->ufo_killed();
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
