#ifndef QCONTROLLINE_H
#define QCONTROLLINE_H

#include <QPushButton>

#include "qbooleanline.h"
#include "ui_qdisplayline.h"

class QControlLine: public QBooleanLine {
    Q_OBJECT
private:
    QPushButton *bt_toggle;
public:
    QControlLine(QWidget *parent);

public slots:
    void set_value(bool new_value);
signals:
    void toggled(void);
};

#endif // QCONTROLLINE_H
