#ifndef QUFOMANAGER_H
#define QUFOMANAGER_H

#include <QGroupBox>
#include <QProcess>

#include "windows.h"
#include "winuser.h"
#include "process.h"

namespace Ui {
    class QUfoManager;
}

class QUfoManager: public QGroupBox {
    Q_OBJECT
private:
    Ui::QUfoManager * ui;
    bool m_autostart;
    QProcess m_process;

private slots:
    void on_cb_auto_clicked(bool checked);

public:
    explicit QUfoManager(QWidget * parent = nullptr);
    ~QUfoManager();

    bool autostart(void) const;
    void set_autostart(bool enable);
signals:
    void started(void) const;
    void stopped(void) const;
};

#endif // QUFOMANAGER_H
