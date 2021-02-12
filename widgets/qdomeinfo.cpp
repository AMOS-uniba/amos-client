#include "qdomeinfo.h"
#include "ui_qdomeinfo.h"

QDomeInfo::QDomeInfo(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::QDomeInfo)
{
    this->ui->setupUi(this);

    this->ui->bl_error_t_lens->set_title("Lens temperature");
    this->ui->bl_error_t_SHT->set_title("Environment temperature");
    this->ui->bl_error_light->set_title("Emergency closing (light)");
    this->ui->bl_error_watchdog->set_title("Watchdog reset");
    this->ui->bl_error_brownout->set_title("Brownout reset");
    this->ui->bl_error_master->set_title("Master power");
    this->ui->bl_error_t_CPU->set_title("CPU temperature");
    this->ui->bl_error_rain->set_title("Emergency closing (rain)");
}

QDomeInfo::~QDomeInfo() {
    delete ui;
}
