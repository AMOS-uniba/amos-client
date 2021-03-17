#include "qcontrolline.h"

#include "qsizepolicy.h"

QControlLine::QControlLine(QWidget *parent):
    QBooleanLine(parent)
{
    this->bt_toggle = new QPushButton(this);
    this->bt_toggle->setText("undefined");
    this->ui->lb_title->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    this->ui->lb_title->setMinimumWidth(100);
    this->ui->lb_value->setMaximumWidth(80);
    this->ui->layout->insertWidget(1, this->bt_toggle);

    this->connect(this->bt_toggle, &QPushButton::clicked, this, &QControlLine::toggled);
}

void QControlLine::set_value(bool new_value) {
    QBooleanLine::set_value(new_value);
    this->bt_toggle->setText(new_value ? "Turn off" : "Turn on");
}

void QControlLine::set_enabled(bool enabled) {
    this->bt_toggle->setEnabled(enabled);
}
