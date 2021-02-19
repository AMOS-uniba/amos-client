#ifndef QDOMEINFO_H
#define QDOMEINFO_H

#include <QGroupBox>
#include "utils/domestate.h"
#include "mainwindow.h"
#include "widgets/domewidget.h"

namespace Ui {
    class QDomeInfo;
}

class QDomeInfo : public QGroupBox {
    Q_OBJECT
private:
    Ui::QDomeInfo *ui;

public:
    explicit QDomeInfo(QWidget *parent = nullptr);
    ~QDomeInfo();

    static QColor temperature_colour(double temperature);

public slots:
    void display_basic_data(const DomeStateS state);
    void display_env_data(const DomeStateT state);
    void display_shaft_data(const DomeStateZ state);

    void set_formatters(double humidity_limit_lower, double humidity_limit_upper);

signals:
    void state_updated_S(void);
    void state_updated_T(void);
    void state_updated_Z(void);

    void cover_closed(int position) const;
    void cover_open(int position) const;
    void cover_moved(int position) const;
};

#endif // QDOMEINFO_H
