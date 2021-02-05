#ifndef QSUNLINE_H
#define QSUNLINE_H

#include <QWidget>
#include <QDateTime>

typedef QColor (*ColourFormatterOld)(double value);

namespace Ui {
    class QSunLine;
}

class QSunLine: public QWidget {
    Q_OBJECT
private:
    Ui::QSunLine *ui;
    ColourFormatterOld m_colour_formatter;

    static QColor m_default_colour(double) { return Qt::black; };
public:
    explicit QSunLine(QWidget *parent = nullptr);
    ~QSunLine();
public slots:
    void set_title(const QString &new_title);
    QString title(void) const;

    void set_value(const double new_value);
    void set_time(const QDateTime &new_datetime);

    void set_colour_formatter(ColourFormatterOld);
};


/*
template <typename T>
using ColorFormatter = const QColor (*)(T value);

template <typename T>
using ValueFormatter = const QString (*)(T value);


ColorFormatter<bool> default_colour_formatter = [](bool) -> const QColor { return Qt::black; };
ValueFormatter<bool> default_formatter = [](bool value) -> const QString { return value ? "on" : "off"; };


template <typename T>
class QDisplayLine: public QWidget {
    Q_OBJECT
private:
    //ColourFormatterX<T> m_colour_formatter;
public:
    explicit QDisplayLine(QWidget *parent = nullptr);
    ~QDisplayLine(void);

    const QString& title(void) const;
    const T& value(void) const;
public slots:
    void set_title(const QString &new_title);
    void set_value(const T& new_value);
};*/

#endif // QSUNLINE_H
