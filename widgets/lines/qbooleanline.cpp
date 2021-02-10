#include "qbooleanline.h"

QBooleanLine::QBooleanLine(
        QWidget *parent,
        ColourFormatter<bool> colour_formatter,
        ValueFormatter<bool> value_formatter):
    QDisplayLine(parent),
    m_colour_formatter(colour_formatter),
    m_value_formatter(value_formatter)
{}

void QBooleanLine::set_value(bool new_value) {
    this->ui->lb_value->setText(this->m_value_formatter(new_value));
}
