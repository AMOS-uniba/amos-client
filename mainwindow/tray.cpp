#include "include.h"
#include "ui_mainwindow.h"

#include <QMenu>

extern EventLogger logger;

void MainWindow::closeEvent(QCloseEvent * event) {
   /* if (this->tray_icon->isVisible()) {
        this->hide();
       // event->ignore();
    }*/

    if (this->m_terminate) {
        this->on_bt_apply_clicked();
        event->accept();
    } else {
        event->ignore();
        QMessageBox * box = new QMessageBox(QMessageBox::Question,
                                            "Confirm exit",
                                            "Are you sure you want to turn off the client?",
                                            QMessageBox::Yes | QMessageBox::No,
                                            this);

        box->connect(box->button(QMessageBox::Yes), &QAbstractButton::clicked, qApp, &QApplication::quit);
        box->connect(box->button(QMessageBox::No), &QAbstractButton::clicked, box, &QObject::deleteLater);
        box->connect(qApp, &QApplication::aboutToQuit, box, &QObject::deleteLater);
        box->exec();
    }
}

void MainWindow::create_tray_icon() {
    this->trayIconMenu = new QMenu(this);
    this->trayIconMenu->addAction(this->minimizeAction);
    this->trayIconMenu->addAction(this->maximizeAction);
    this->trayIconMenu->addAction(this->restoreAction);
    this->trayIconMenu->addSeparator();
    this->trayIconMenu->addAction(this->quitAction);

    this->tray_icon = new QSystemTrayIcon(this);
    this->tray_icon->setContextMenu(this->trayIconMenu);
}

void MainWindow::create_actions() {
    this->minimizeAction = new QAction("Mi&nimize", this);
    this->connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    this->maximizeAction = new QAction("Ma&ximize", this);
    this->connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    this->restoreAction = new QAction("&Restore", this);
    this->connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    this->quitAction = new QAction("&Quit", this);
    this->connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void MainWindow::set_icon(const StationState & state) {
    this->tray_icon->setIcon(this->icons[state.icon()]);
    this->tray_icon->setToolTip(QString("AMOS client\n%1\nsince %2").arg(
        state.display_string(),
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
    ));
}

void MainWindow::icon_activated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
        case QSystemTrayIcon::DoubleClick:
            if (this->isVisible()) {
                this->hide();
            } else {
                this->show();
                this->activateWindow();
            }
            break;
        default:
        case QSystemTrayIcon::MiddleClick:
            break;
    }
}

void MainWindow::show_message(void) {
    this->tray_icon->showMessage(
        "AMOS client",
        this->ui->station->state().tooltip(),
        this->icons[this->ui->station->state().icon()],
        3000
    );
}
