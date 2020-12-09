#include "include.h"

#include <QMenu>

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
    QIcon icon;
    QString tooltip;
    switch (state) {
        case StationState::MANUAL:
            icon = QIcon(":/images/green.ico");
            tooltip = "Manual control enabled";
            break;
        case StationState::DAY:
            icon = QIcon(":/images/yellow.ico");
            tooltip = "Day, not observing";
            break;
        case StationState::OBSERVING:
            icon = QIcon(":/images/blue.ico");
            tooltip = "Observation in progress";
            break;
        case StationState::NOT_OBSERVING:
            icon = QIcon(":/images/grey.ico");
            tooltip = "Not observing";
            break;
        case StationState::DOME_UNREACHABLE:
            icon = QIcon(":/images/red.ico");
            tooltip = "Dome is not responding";
            break;
    }

    this->tray_icon->setIcon(icon);
    this->tray_icon->setToolTip(QString("AMOS controller\n%1").arg(tooltip));
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
    switch (this->station->state()) {
        case StationState::DAY:
            this->tray_icon->showMessage("AMOS controller", "AMOS is working", QIcon(":/images/images/blue.ico"), 5000);
            break;
        default:
            break;
    }
}
