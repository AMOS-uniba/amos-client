#ifndef QSUNINFO_H
#define QSUNINFO_H

#include <QGroupBox>
#include <QTimer>

#include "utils/universe.h"
#include "widgets/qcamera.h"

namespace Ui {
    class QSunInfo;
}

class QSunInfo: public QGroupBox {
    Q_OBJECT
private:
    Ui::QSunInfo * ui;
    const QStation * m_station;
    const QCamera * m_allsky_camera;

    QTimer * m_timer_short;
    QTimer * m_timer_long;
public:
    explicit QSunInfo(QWidget * parent = nullptr);
    ~QSunInfo();
    void set_station(const QStation * const station);
    void set_allsky_camera(const QCamera * const camera);

public slots:
    void update_short_term(void);
    void update_long_term(void);
};

#endif // QSUNINFO_H
