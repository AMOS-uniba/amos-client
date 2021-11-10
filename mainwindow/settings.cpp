#include "ui_mainwindow.h"

extern EventLogger logger;
extern QSettings * settings;

void MainWindow::load_settings(void) {
    try {
        logger.info(Concern::Operation, QString("Reading settings from \"%1\"").arg(settings->fileName()));

        this->ui->station->set_server(this->ui->server);
        this->ui->station->set_cameras(this->ui->camera_allsky, this->ui->camera_spectral);
        this->ui->station->set_dome(this->ui->dome);

        logger.load_settings(settings);

        // Load and set debug levels
        bool debug = settings->value("debug", false).toBool();
        logger.set_level(debug ? Level::Debug : Level::Info);
        this->ui->cb_debug->setChecked(debug);
    } catch (ConfigurationError & e) {
        QString postmortem = QString("Fatal configuration error: %1").arg(e.what());
        QMessageBox box;
        box.setText(postmortem);
        box.setIcon(QMessageBox::Icon::Critical);
        box.setWindowIcon(QIcon(":/images/blue.ico"));
        box.setWindowTitle("Configuration error");
        box.setStandardButtons(QMessageBox::Ok);
        box.exec();

        logger.fatal(Concern::Configuration, postmortem);
        exit(-4);
    }
}

void MainWindow::slot_settings_changed() {
    bool changed =
        this->ui->dome->is_changed() ||
        this->ui->station->is_changed() ||
        this->ui->server->is_changed() ||
        this->ui->camera_allsky->is_changed() ||
        this->ui->camera_spectral->is_changed();

    if (changed) {
        this->ui->bt_apply->setEnabled(true);
        this->ui->bt_apply->setText("Apply changes");
        this->ui->bt_discard->setEnabled(true);
    } else {
        this->ui->bt_apply->setEnabled(false);
        this->ui->bt_apply->setText("No changes");
        this->ui->bt_discard->setEnabled(false);
    }
}

void MainWindow::on_bt_apply_clicked() {
    foreach (QAmosWidget * widget, this->amos_widgets) {
        widget->apply_changes();
        widget->save_settings();
    }
}

void MainWindow::on_bt_discard_clicked() {
    foreach (QAmosWidget * widget, this->amos_widgets) {
        widget->discard_changes();
    }
}
