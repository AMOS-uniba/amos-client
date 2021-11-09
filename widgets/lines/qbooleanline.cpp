#include "qbooleanline.h"

QBooleanLine::QBooleanLine(
        QWidget * parent,
        ColourFormatter<bool> colour_formatter,
        ValueFormatter<bool> value_formatter):
    QDisplayLine(parent),
    m_colour_formatter(colour_formatter),
    m_value_formatter(value_formatter)
{}

void QBooleanLine::set_colour_formatter(ColourFormatter<bool> new_colour_formatter) {
    this->m_colour_formatter = new_colour_formatter;
}

void QBooleanLine::set_value_formatter(ValueFormatter<bool> new_value_formatter) {
    this->m_value_formatter = new_value_formatter;
}

void QBooleanLine::set_formatters(QColor colour_on, QColor colour_off, const QString & value_on, const QString & value_off) {
    this->set_colour_formatter([=](bool value) -> QColor { return value ? colour_on : colour_off; });
    this->set_value_formatter([=](bool value) -> QString { return value ? value_on : value_off; });
}

void QBooleanLine::set_value(bool new_value) {
    if (this->m_valid) {
        this->ui->lb_value->setText(this->m_value_formatter(new_value));
        this->ui->lb_value->setStyleSheet(QString("QLabel { color: %1; }").arg(this->m_colour_formatter(new_value).name()));
    } else {
        this->ui->lb_value->setText("?");
        this->ui->lb_value->setStyleSheet("QLabel { color: '#7F7F7F'; }");
    }
}
