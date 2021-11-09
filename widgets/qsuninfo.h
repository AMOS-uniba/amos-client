#ifndef QSUNINFO_H
#define QSUNINFO_H

#include <QGroupBox>
#include <QTimer>

#include "utils/universe.h"
#include "utils/formatters.h"
#include "widgets/qcamera.h"

namespace Ui {
    class QSunInfo;
}

class QSunInfo: public QGroupBox {
    Q_OBJECT
private:
    Ui::QSunInfo * ui;
    const QStation * m_station;

    QTimer * m_timer_short;
    QTimer * m_timer_long;
public:
    explicit QSunInfo(QWidget * parent = nullptr);
    ~QSunInfo();
    void set_station(const QStation * const station);

public slots:
    void update_short_term(void);
    void update_long_term(void);
};

#endif // QSUNINFO_H
