#include "qaboutdialog.h"
#include "ui_qaboutdialog.h"


QAboutDialog::QAboutDialog(QWidget * parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    this->ui->setupUi(this);
    this->ui->lb_datetime->setText(QString("Built on %1 %2").arg(__DATE__, __TIME__));
#if PROTOCOL == 2015
    this->ui->lb_protocol->setText("Compiled for old protocol (Senec)");
    this->ui->lb_version->setText(QString("Version %1%2").arg(VERSION_STRING).arg("sc"));
#elif PROTOCOL == 2020
    this->ui->lb_protocol->setText("Compiled for new protocol");
    this->ui->lb_version->setText(QString("Version %1").arg(VERSION_STRING));
#endif
}

QAboutDialog::~QAboutDialog() {
    delete this->ui;
}

void QAboutDialog::on_buttonBox_accepted() {
    this->close();
}
