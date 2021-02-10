#include "qdisplayline.h"
#include "ui_qdisplayline.h"

QDisplayLine::QDisplayLine(QWidget *parent):
    QWidget(parent),
    ui(new Ui::QDisplayLine)
{
    ui->setupUi(this);
}

QDisplayLine::~QDisplayLine() {
    delete ui;
}

void QDisplayLine::set_title(const QString &new_title) {
    this->ui->lb_title->setText(new_title);
}

QString QDisplayLine::title(void) const {
    return this->ui->lb_title->text();
}
