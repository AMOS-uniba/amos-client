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


#endif // QDISPLAYLINE_H
