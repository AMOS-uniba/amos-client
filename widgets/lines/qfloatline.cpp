#include "qfloatline.h"

QFloatLine::QFloatLine(
            QWidget *parent,
            ColourFormatter<double> colour_formatter,
            ValueFormatter<double> value_formatter):
    QDisplayLine(parent),
    m_colour_formatter(colour_formatter),
    m_value_formatter(value_formatter)
{}

void QFloatLine::set_colour_formatter(ColourFormatter<double> new_colour_formatter) {
    this->m_colour_formatter = new_colour_formatter;
}

void QFloatLine::set_value_formatter(ValueFormatter<double> new_value_formatter) {
    this->m_value_formatter = new_value_formatter;
}

void QFloatLine::set_value(double new_value) {
    if (this->m_valid) {
        this->ui->lb_value->setText(this->m_value_formatter(new_value));
        this->ui->lb_value->setStyleSheet(QString("QLabel { color: %1; }").arg(this->m_colour_formatter(new_value).name()));
    } else {
        this->ui->lb_value->setText("?");
        this->ui->lb_value->setStyleSheet("QLabel { color: '#7F7F7F'; }");
    }
}
