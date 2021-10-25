#ifndef QDISPLAYLINE_H
#define QDISPLAYLINE_H

#include <QWidget>
#include <QDateTime>

namespace Ui {
    class QDisplayLine;
}

template <typename T>
using ColourFormatter = std::function<QColor(T value)>;

template <typename T>
using ValueFormatter = std::function<QString(T value)>;


class QDisplayLine: public QWidget {
    Q_OBJECT
protected:
    Ui::QDisplayLine * ui;
    bool m_valid;
public:
    explicit QDisplayLine(QWidget * parent = nullptr);
    ~QDisplayLine();

public slots:
    void set_title(const QString & new_title);
    void set_tooltip(const QString & new_tooltip);
    void set_valid(bool valid);
};


#endif // QDISPLAYLINE_H
