#include "qsunline.h"
#include "ui_qsunline.h"

QSunLine::QSunLine(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QSunLine)
{
    ui->setupUi(this);
    this->set_title("Undefined");
    this->set_colour_formatter(&this->m_default_colour);
}

QSunLine::~QSunLine() {
    delete ui;
}

void QSunLine::set_title(const QString &new_title) {
    this->ui->lb_title->setText(new_title);
}

QString QSunLine::title(void) const {
    return this->ui->lb_title->text();
}

void QSunLine::set_value(const double new_value) {
    this->ui->lb_value->setText(QString("%1°").arg(new_value, 3, 'f', 3));
    this->ui->lb_value->setStyleSheet(QString("QLabel { color: %1; }").arg(m_colour_formatter(new_value).name()));
}

void QSunLine::set_time(const QDateTime &new_datetime) {
    this->ui->lb_value->setText(new_datetime.toString("hh:mm"));
}

void QSunLine::set_colour_formatter(ColourFormatterOld formatter) {
    this->m_colour_formatter = formatter;
}
