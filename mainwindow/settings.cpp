#include "include.h"
#include "ui_mainwindow.h"

extern EventLogger logger;
extern QSettings *settings;

void MainWindow::load_settings(void) {
    try {
        logger.info(Concern::Operation, QString("Reading settings from \"%1\"").arg(settings->fileName()));

        this->ui->station->set_server(this->ui->server);
        this->ui->station->set_storages(this->ui->storage_primary, this->ui->storage_permanent);
        this->ui->station->set_dome(this->ui->dome);
        this->ui->station->set_ufo_manager(this->ui->ufo_manager);

        this->ui->scanner->set_directory(QDir(settings->value("storage/scanner_path", "C:\\Data").toString()));
        this->ui->scanner->set_enabled(settings->value("storage/scanner_enabled", true).toBool());
        this->ui->scanner->scan_info();

        this->ui->storage_primary->set_directory(QDir(settings->value("storage/primary_path", "C:\\Data").toString()));
        this->ui->storage_primary->set_enabled(settings->value("storage/primary_enabled", true).toBool());
        this->ui->storage_primary->scan_info();

        this->ui->storage_permanent->set_directory(QDir(settings->value("storage/permanent_path", "D:\\Data").toString()));
        this->ui->storage_permanent->set_enabled(settings->value("storage/permanent_enabled", true).toBool());
        this->ui->storage_permanent->scan_info();

        logger.load_settings();

        // Load and set debug levels
        bool debug = settings->value("debug", false).toBool();
        logger.set_level(debug ? Level::Debug : Level::Info);
        this->ui->cb_debug->setChecked(debug);
    } catch (ConfigurationError &e) {
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
