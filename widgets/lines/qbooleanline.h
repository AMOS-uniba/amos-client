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
            QWidget *parent = nullptr,
            ColourFormatter<bool> colour_formatter = [](bool value) -> QColor { return value ? Qt::red : Qt::black; },
            ValueFormatter<bool> value_formatter = [](bool value) -> QString { return value ? "on" : "off"; }
    );
public slots:
    void set_enabled(bool enabled);
    void set_value(bool new_value);
};

#endif // QBOOLEANLINE_H
