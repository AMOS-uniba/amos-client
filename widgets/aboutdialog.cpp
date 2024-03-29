#include "aboutdialog.h"
#include "ui_aboutdialog.h"


AboutDialog::AboutDialog(QWidget * parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    this->ui->setupUi(this);
    this->ui->lb_datetime->setText(QString("Built on %1 %2").arg(__DATE__, __TIME__));
    this->ui->lb_protocol->setText(
#if OLD_PROTOCOL
    "Compiled for old protocol (Senec)"
#else
    "Compiled for new protocol"
#endif
    );
    this->ui->lb_version->setText(QString("Version %1").arg(VERSION_STRING));
}

AboutDialog::~AboutDialog() {
    delete this->ui;
}

void AboutDialog::on_buttonBox_accepted() {
    this->close();
}
