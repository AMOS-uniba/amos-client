#ifndef QDISPLAYLINE_H
#define QDISPLAYLINE_H

#include <QWidget>
#include <QDateTime>

namespace Ui {
    class QDisplayLine;
}

template <typename T>
using ColourFormatter = QColor (*)(T value);

template <typename T>
using ValueFormatter = QString (*)(T value);


class QDisplayLine: public QWidget {
    Q_OBJECT
protected:
    Ui::QDisplayLine *ui;
public:
    explicit QDisplayLine(QWidget *parent = nullptr);
    ~QDisplayLine();

    QString title(void) const;

public slots:
    void set_title(const QString &new_title);
};


class QDateTimeLine: public QDisplayLine {
    Q_OBJECT
public:
    explicit QDateTimeLine(
            QWidget *parent = nullptr,
            ColourFormatter<QDateTime> colour_formatter = [](QDateTime) -> QColor { return Qt::black; },
            ValueFormatter<QDateTime> value_formatter = [](QDateTime value) -> QString { return value.toString("hh:mm"); }
    );
};


class QBooleanLine: public QDisplayLine {
    Q_OBJECT
public:
    explicit QBooleanLine(
            QWidget *parent = nullptr,
            ColourFormatter<bool> colour_formatter = [](bool value) -> QColor { return value ? Qt::red : Qt::black; },
            ValueFormatter<bool> value_formatter = [](bool value) -> QString { return value ? "on" : "off"; }
    );
};


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
public slots:
    void set_value(double new_value);
};

#endif // QDISPLAYLINE_H
