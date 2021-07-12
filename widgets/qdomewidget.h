#ifndef QDOMEWIDGET_H
#define QDOMEWIDGET_H

#include <QObject>
#include <QWidget>
#include <QtGui>

#include "logging/eventlogger.h"

class QDomeWidget: public QWidget {
    Q_OBJECT
private:
    int m_cover_position;
    int m_cover_minimum;
    int m_cover_maximum;
    bool m_reachable;

protected:
    void paintEvent(QPaintEvent * e) override;
    void drawWidget(QPainter & qp);

public:
    QDomeWidget(QWidget * parent = nullptr);

public slots:
    void set_cover_position(int new_position);
    void set_cover_minimum(int new_minimum);
    void set_cover_maximum(int new_maximum);
    void set_reachable(bool reachable);
};

#endif // QDOMEWIDGET_H
