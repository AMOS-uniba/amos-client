#ifndef QDATETIMELINE_H
#define QDATETIMELINE_H

#include "qdisplayline.h"
#include "ui_qdisplayline.h"

class QDateTimeLine: public QDisplayLine {
    Q_OBJECT
private:
    ColourFormatter<QDateTime> m_colour_formatter;
    ValueFormatter<QDateTime> m_value_formatter;

public:
    explicit QDateTimeLine(
        QWidget * parent = nullptr,
        ColourFormatter<QDateTime> colour_formatter = [](QDateTime) -> QColor { return Qt::black; },
        ValueFormatter<QDateTime> value_formatter = [](QDateTime value) -> QString {
            return value.isValid() ? value.toString("hh:mm") : "--:--";
        }
    );

public slots:
    void set_value(const QDateTime & new_value);
};

#endif // QDATETIMELINE_H
