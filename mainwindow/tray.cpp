#include "include.h"

#include <QMenu>

extern EventLogger logger;

void MainWindow::closeEvent(QCloseEvent *event) {
   /* if (this->tray_icon->isVisible()) {
        this->hide();
       // event->ignore();
    }*/

    event->ignore();
    if (QMessageBox::question(this, "Confirm exit", "Are you sure you want to turn off the client?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        event->accept();
    }
}
void MainWindow::create_tray_icon() {
    this->trayIconMenu = new QMenu(this);
    this->trayIconMenu->addAction(minimizeAction);
    this->trayIconMenu->addAction(maximizeAction);
    this->trayIconMenu->addAction(restoreAction);
    this->trayIconMenu->addSeparator();
    this->trayIconMenu->addAction(quitAction);

    this->tray_icon = new QSystemTrayIcon(this);
    this->tray_icon->setContextMenu(trayIconMenu);

    logger.info(Concern::Configuration, "Tray icon created");
}

void MainWindow::create_actions() {
    minimizeAction = new QAction("Mi&nimize", this);
    connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    maximizeAction = new QAction("Ma&ximize", this);
    connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    restoreAction = new QAction("&Restore", this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction("&Quit", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void MainWindow::set_icon(const StationState &state) {
    logger.warning(Concern::Operation, QString("Icon '%1' %2").arg(QString(state.code()), state.tooltip()));
    this->tray_icon->setIcon(QIcon(":/images/green.ico"));//state.icon());
    this->tray_icon->setToolTip(QString("AMOS client\n%1").arg(state.tooltip()));
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
        case QSystemTrayIcon::MiddleClick:
            break;
        default:
            break;
    }
}

void MainWindow::show_message(void) {
    this->tray_icon->showMessage("AMOS controller", this->station->state().tooltip(), this->station->state().icon(), 5000);
}