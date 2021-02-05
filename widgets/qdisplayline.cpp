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

QFloatLine::QFloatLine(
            QWidget *parent,
            ColourFormatter<double> colour_formatter,
            ValueFormatter<double> value_formatter):
    QDisplayLine(parent),
    m_colour_formatter(colour_formatter),
    m_value_formatter(value_formatter)
{}

void QFloatLine::set_value(double new_value) {
    this->ui->lb_value->setText(this->m_value_formatter(new_value));
}
