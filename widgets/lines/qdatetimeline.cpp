#include "qdisplayline.h"
#include "qdatetimeline.h"

QDateTimeLine::QDateTimeLine(
        QWidget *parent,
        ColourFormatter<QDateTime> colour_formatter,
        ValueFormatter<QDateTime> value_formatter):
    QDisplayLine(parent),
    m_colour_formatter(colour_formatter),
    m_value_formatter(value_formatter)
{}

void QDateTimeLine::set_value(const QDateTime &new_value) {
    this->ui->lb_value->setText(this->m_value_formatter(new_value));
}
