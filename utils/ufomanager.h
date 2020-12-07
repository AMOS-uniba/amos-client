#ifndef UFOMANAGER_H
#define UFOMANAGER_H

#include "forward.h"
#include "windows.h"
#include "winuser.h"
#include "process.h"

#include <QProcess>

enum class UfoState {
    NOT_FOUND,
    NOT_EXE,
    NOT_RUNNING,
    RUNNING
};


class UfoManager: public QObject {
    Q_OBJECT
private:
    HWND m_frame;
    QString m_path;
    qint64 m_pid;

    UfoState m_state;
public:
    UfoManager();
    ~UfoManager(void);

    UfoState state(void) const;
    QString state_string(void) const;

    bool is_running(void);
    void start_ufo(void);
    void kill_ufo(void);

    qint64 pid(void) const;

    void set_path(const QString &path);
    const QString& path() const;
};

#endif // UFOMANAGER_H