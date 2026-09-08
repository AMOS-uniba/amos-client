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
const UfoState QUfoManager::Stopping     = UfoState('T', "stopping", QColor::fromHsl(30, 255, 128), false, "Stopping...");

const QString QUfoManager::DefaultPathAllSky = "C:/AMOS/UFO2/UFO2.exe";
const QString QUfoManager::DefaultPathSpectral = "C:/AMOS/UFOHD2/UFOHD2.exe";

namespace {
    struct MainWindowSearch {
        DWORD pid = 0;
        HWND found = nullptr;
    };

    BOOL CALLBACK match_main_window(HWND window, LPARAM parameter) {
        MainWindowSearch * search = reinterpret_cast<MainWindowSearch *>(parameter);

        DWORD pid = 0;
        GetWindowThreadProcessId(window, &pid);
        if (pid != search->pid) {
            return TRUE;
        }

        /* A process owns far more windows than its main frame, and the frame has to be picked out
         * of them. Measured against both detectors on a real machine:
         *
         *   owned windows are tooltips, IME helpers and dialogs, never the frame;
         *   no caption or no system menu rules out tooltips and UFO's five stray ComboLBoxes,
         *     which are unowned and captioned but are list boxes;
         *   visibility is what finally separates Kvant's frame (titled "AMOS") from its own
         *     hidden_device_change_window, which has an identical style profile -- unowned,
         *     captioned, with a system menu.
         *
         * Not filtered on WS_POPUP: UFOCapture's main window is of dialog class #32770 and does
         * carry it, so excluding popups would lose the very window this used to find.
         */
        if (GetWindow(window, GW_OWNER) != nullptr) {
            return TRUE;
        }

        const LONG_PTR style = GetWindowLongPtr(window, GWL_STYLE);
        if (((style & WS_CAPTION) == 0) || ((style & WS_SYSMENU) == 0)) {
            return TRUE;
        }

        if (!IsWindowVisible(window)) {
            return TRUE;
        }

        search->found = window;
        return FALSE;
    }
}

/**
 * @brief Find the main window of a process by its id.
 *
 * Not by title. This used to be FindWindowA(nullptr, "UFOCapture"), which only ever worked for
 * UFOCapture -- and now that a station may run Kvant instead, whose executable is AMOS64.exe and
 * whose window is titled something else entirely, it would simply never find anything. The station
 * would still observe, but the window would not be minimised and every stop would fall through to
 * killing the process.
 *
 * The process id is the reliable handle: this class starts the program itself, so it knows which
 * process to ask about, and nothing has to be configured or guessed per detector. The one case it
 * cannot follow is a launcher that exits and leaves the real program in another process, which
 * neither UFO2.exe nor AMOS64.exe does.
 *
 * Verified against both: UFOCapture's window is found in about 200 ms, and it is the same window
 * the old title search found. Kvant creates its frame straight away but leaves it hidden on a
 * machine with no camera attached, so there it is not found and the caller carries on without one
 * -- exactly as it does for a UFO that never opened a window.
 */
HWND QUfoManager::main_window_of(qint64 pid) {
    if (pid <= 0) {
        return nullptr;
    }

    MainWindowSearch search;
    search.pid = static_cast<DWORD>(pid);
    EnumWindows(match_main_window, reinterpret_cast<LPARAM>(&search));
    return search.found;
}

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
    this->m_timer_delay->setSingleShot(true);
    // Connected once. It used to be connected on every scheduled start, with no Qt::UniqueConnection,
    // so the connections accumulated and after a week of dusk starts a single timeout launched UFO
    // many times over.
    this->connect(this->m_timer_delay, &QTimer::timeout, this, &QUfoManager::start_ufo_inner);

    this->m_timer_sequence = new QTimer(this);
    this->m_timer_sequence->setSingleShot(true);
    this->m_timer_sequence->setInterval(QUfoManager::PollInterval);
    this->connect(this->m_timer_sequence, &QTimer::timeout, this, &QUfoManager::run_step);

    this->connect(this, &QUfoManager::state_changed, this, &QUfoManager::log_state_change);
}

QUfoManager::~QUfoManager() {
    this->disconnect(&this->m_process, &QProcess::stateChanged, nullptr, nullptr);
    delete this->m_timer_check;
    delete this->m_timer_delay;
    delete this->m_timer_sequence;
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
                    if (open_since.isValid()) {
                        logger.debug(Concern::UFO, QString("Camera %1: Cover open and II active for %2 s")
                            .arg(this->id())
                            .arg(open_since.secsTo(QDateTime::currentDateTimeUtc()))
                        );
                        if (open_since.secsTo(QDateTime::currentDateTimeUtc()) > QUfoManager::OpenDelay) {
                            this->start_ufo();
                        }
                    } else {
                        //logger.debug(Concern::UFO, QString("Camera %1: Dome is not open or II is off, stopping UFO").arg(this->id()));
                        //this->stop_ufo();
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

    UfoState old_ufo_state = this->m_state;
    UfoState new_ufo_state = QUfoManager::Unknown;

    switch (this->m_process.state()) {
        case QProcess::ProcessState::Running: {
            this->connect(this->ui->bt_toggle, &QPushButton::clicked, this, &QUfoManager::stop_ufo);
            // The one-second poll must not overwrite a stop sequence that is still in flight.
            new_ufo_state = this->is_stopping() ? QUfoManager::Stopping : QUfoManager::Running;
            break;
        }
        case QProcess::ProcessState::Starting: {
            new_ufo_state = QUfoManager::Starting;
            break;
        }
        case QProcess::ProcessState::NotRunning: {
            QFileInfo info(this->path());
            if (info.exists()) {
                if (this->path().endsWith(".exe") && info.isFile()) {
                    this->connect(this->ui->bt_toggle, &QPushButton::clicked, this, &QUfoManager::start_ufo);
                    new_ufo_state = QUfoManager::NotRunning;
                } else {
                    new_ufo_state = QUfoManager::NotAnExe;
                }
            } else {
                new_ufo_state = QUfoManager::NotFound;
            }
            break;
        }
    }
    logger.debug(Concern::UFO, QString("UFO-%1 state is %2").arg(this->id(), new_ufo_state.display_string()));

    this->ui->lb_state->setText(new_ufo_state.display_string());
    this->ui->lb_state->setStyleSheet(QString("QLabel { color: %1; }").arg(new_ufo_state.colour().name()));
    this->ui->bt_toggle->setEnabled(new_ufo_state.button_enabled());
    this->ui->bt_toggle->setText(new_ufo_state.button_text());
    this->ui->cb_auto->setEnabled(new_ufo_state.button_enabled());

    if (new_ufo_state != this->m_state) {
        this->m_state = new_ufo_state;
        emit this->state_changed(new_ufo_state);
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
                this->m_timer_delay->start(delay * 1000);
            }
            break;
        }
    }
}

/**
 * @brief Schedule the next step of a start or stop sequence.
 * Resets the attempt counter whenever the step actually changes, so each phase gets its own budget.
 */
void QUfoManager::advance(Step step) {
    if (step != this->m_step) {
        this->m_step = step;
        this->m_attempts = 0;
    } else {
        this->m_attempts += 1;
    }
    this->m_timer_sequence->start(QUfoManager::PollInterval);
}

/**
 * @brief QUfoManager::start_ufo_inner
 * Actually starts UFO, private function
 */
void QUfoManager::start_ufo_inner(void) {
    logger.debug(Concern::UFO, QString("UFO-%1 starting").arg(this->id()));
    this->m_process.setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedChannels);
    this->m_process.setWorkingDirectory(QFileInfo(this->m_path).absoluteDir().path());
    this->m_process.start(this->m_path, {}, QProcess::OpenMode(QProcess::ReadWrite));

    this->advance(Step::FindWindow);
}

/**
 * @brief QUfoManager::stop_ufo
 * Ask UFO to close, politely, without blocking. The sequence continues in run_step().
 */
void QUfoManager::stop_ufo(void) {
    if (!this->is_running()) {
        logger.debug(Concern::UFO, QString("UFO-%1: Application is not running, not doing anything").arg(this->id()));
        return;
    }

    if (this->is_stopping()) {
        logger.debug(Concern::UFO, QString("UFO-%1: already stopping").arg(this->id()));
        return;
    }

    logger.info(Concern::UFO, QString("UFO-%1 stopping").arg(this->id()));

    if (!IsWindow(this->m_frame)) {
        logger.warning(Concern::UFO, QString("UFO-%1: HWND is invalid, killing process").arg(this->id()));
        this->m_process.kill();
        emit this->stopped();
        return;
    }

    // Posted, not sent: SendMessage waits for UFO's own WndProc to finish, which is most of the
    // freeze this sequence exists to remove.
    PostMessage(this->m_frame, WM_CLOSE, 0, 0);
    this->advance(Step::AwaitPopup);
}

/**
 * @brief One step of whichever sequence is in flight.
 * Each phase polls up to its own budget instead of sleeping for a fixed guess, so a slow machine gets
 * more time and a fast one wastes none.
 */
void QUfoManager::run_step(void) {
    switch (this->m_step) {
        case Step::Idle: {
            break;
        }
        case Step::FindWindow: {
            this->m_frame = QUfoManager::main_window_of(this->m_process.processId());
            if (this->m_frame != nullptr) {
                logger.debug(Concern::UFO, QString("UFO-%1 HWND is %2, minimising")
                                               .arg(this->id()).arg((long long) this->m_frame));
                ShowWindowAsync(this->m_frame, SW_SHOWMINIMIZED);
            } else if (this->m_attempts < QUfoManager::FindWindowAttempts) {
                this->advance(Step::FindWindow);
                return;
            } else {
                // The process is up regardless; without a window we simply cannot minimise it, and
                // stop_ufo() will fall back to killing it. The old code did not check at all and
                // handed a null handle to ShowWindowAsync.
                logger.warning(Concern::UFO, QString("UFO-%1: window did not appear after %2 ms")
                                                 .arg(this->id())
                                                 .arg(QUfoManager::FindWindowAttempts * QUfoManager::PollInterval));
            }
            this->m_step = Step::Idle;
            this->m_start_scheduled = false;
            emit this->started();
            break;
        }
        case Step::AwaitPopup: {
            HWND child = GetLastActivePopup(this->m_frame);
            if ((child != nullptr) && (child != this->m_frame)) {
                logger.debug(Concern::UFO, QString("UFO-%1: confirmation dialog %2, clicking OK")
                                               .arg(this->id()).arg((long long) child));
                SetForegroundWindow(child);
                if (HWND ok = GetDlgItem(child, 1)) {
                    PostMessage(ok, BM_CLICK, 0, 0);
                }
                this->advance(Step::AwaitExit);
            } else if (this->m_attempts < QUfoManager::PopupAttempts) {
                this->advance(Step::AwaitPopup);
            } else {
                logger.debug(Concern::UFO, QString("UFO-%1: no confirmation dialog appeared").arg(this->id()));
                this->advance(Step::AwaitExit);
            }
            break;
        }
        case Step::AwaitExit: {
            if (!this->is_running()) {
                logger.info(Concern::UFO, QString("UFO-%1 closed").arg(this->id()));
                this->m_step = Step::Idle;
                emit this->stopped();
            } else if (this->m_attempts < QUfoManager::ExitAttempts) {
                this->advance(Step::AwaitExit);
            } else {
                logger.debug(Concern::UFO, QString("UFO-%1: still running, posting WM_QUIT").arg(this->id()));
                PostThreadMessage(GetWindowThreadProcessId(this->m_frame, nullptr), WM_QUIT, 0, 0);
                this->advance(Step::AwaitExitAfterQuit);
            }
            break;
        }
        case Step::AwaitExitAfterQuit: {
            if (!this->is_running()) {
                logger.info(Concern::UFO, QString("UFO-%1 closed after WM_QUIT").arg(this->id()));
                this->m_step = Step::Idle;
                emit this->stopped();
            } else if (this->m_attempts < QUfoManager::ExitAttempts) {
                this->advance(Step::AwaitExitAfterQuit);
            } else {
                // Deliberately not killed: that decision predates this change and is left alone.
                logger.warning(Concern::UFO, QString("UFO-%1 would not close; leaving it alone").arg(this->id()));
                this->m_step = Step::Idle;
                emit this->stopped();
            }
            break;
        }
    }
}

void QUfoManager::log_state_change(const UfoState & state) const {
    logger.debug(Concern::UFO, QString("UFO-%1: State changed to \"%2\"").arg(this->id(), state.display_string()));
}

QJsonObject QUfoManager::json(void) const {
    /* 'T' for stopping is new: the server's watcher vocabulary was D/F/E/R/N/S/U, and it needs a
     * WATCHER_STOPPING to go with it. Reporting it before the server knows it is deliberate and
     * safe -- the ingest stores the code without validating it against the choices, verified
     * against the live instance -- so the state simply shows as an unlabelled 'T' until that side
     * catches up. Telling it 'R' instead would have hidden a station stuck mid-shutdown, which is
     * exactly what one wants to see.
     */
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
