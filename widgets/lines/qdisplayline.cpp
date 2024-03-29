#include "qdisplayline.h"
#include "ui_qdisplayline.h"

QDisplayLine::QDisplayLine(QWidget * parent):
    QWidget(parent),
    ui(new Ui::QDisplayLine),
    m_valid(false)
{
    ui->setupUi(this);
    this->setMinimumHeight(22);
}

QDisplayLine::~QDisplayLine() {
    delete ui;
}

void QDisplayLine::set_title(const QString & new_title) {
    this->ui->lb_title->setText(new_title);
}

void QDisplayLine::set_tooltip(const QString & new_tooltip) {
    this->setToolTip(new_tooltip);
}

void QDisplayLine::set_valid(bool valid) {
    this->m_valid = valid;
    this->setEnabled(valid);

    if (!valid) {
        this->ui->lb_value->setText("?");
    }
}
