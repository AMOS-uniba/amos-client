#ifndef QBOOLEANLINE_H
#define QBOOLEANLINE_H

#include "qdisplayline.h"
#include "ui_qdisplayline.h"


class QBooleanLine: public QDisplayLine {
    Q_OBJECT
private:
    ColourFormatter<bool> m_colour_formatter;
    ValueFormatter<bool> m_value_formatter;
public:
    explicit QBooleanLine(
        QWidget * parent = nullptr,
        ColourFormatter<bool> colour_formatter = [](bool value) -> QColor { return value ? Qt::red : Qt::black; },
        ValueFormatter<bool> value_formatter = [](bool value) -> QString { return value ? "on" : "off"; }
    );
    void set_formatters(QColor colour_on, QColor colour_off, const QString & value_on, const QString & value_off);
    void set_colour_formatter(ColourFormatter<bool> new_colour_formatter);
    void set_value_formatter(ValueFormatter<bool> new_value_formatter);
public slots:
    void set_value(bool new_value);
};

#endif // QBOOLEANLINE_H
