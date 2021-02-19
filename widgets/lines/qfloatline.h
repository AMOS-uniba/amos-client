#ifndef QFLOATLINE_H
#define QFLOATLINE_H

#include "qdisplayline.h"
#include "ui_qdisplayline.h"

class QFloatLine: public QDisplayLine {
    Q_OBJECT
private:
    ColourFormatter<double> m_colour_formatter;
    ValueFormatter<double> m_value_formatter;
public:
    explicit QFloatLine(
            QWidget *parent = nullptr,
            ColourFormatter<double> colour_formatter = [](double) -> QColor { return Qt::black; },
            ValueFormatter<double> value_formatter = [](double value) -> QString { return QString("%1°").arg(value, 3, 'f', 3); }
    );

    void set_colour_formatter(ColourFormatter<double> new_colour_formatter);
    void set_value_formatter(ValueFormatter<double> new_value_formatter);
public slots:
    void set_value(double new_value);
};

#endif // QFLOATLINE_H
