#ifndef DOMEWIDGET_H
#define DOMEWIDGET_H

#include <QObject>
#include <QWidget>
#include <QtGui>

class DomeWidget: public QWidget {
    Q_OBJECT
private:
    int m_cover_position;
    int m_cover_minimum;
    int m_cover_maximum;
protected:
    void paintEvent(QPaintEvent *e) override;
    void drawWidget(QPainter &qp);

public:
    DomeWidget(QWidget *parent = nullptr);
public slots:
    void set_cover_position(int new_position);
    void set_cover_minimum(int new_minimum);
    void set_cover_maximum(int new_maximum);
};

#endif // DOMEWIDGET_H
