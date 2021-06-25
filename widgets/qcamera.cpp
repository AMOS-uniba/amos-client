#include "qcamera.h"
#include "ui_qcamera.h"

QCamera::QCamera(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QCamera)
{
    this->ui->setupUi(this);
}

QCamera::~QCamera() {
    delete this->ui;
}
