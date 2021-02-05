#ifndef SUNINFO_H
#define SUNINFO_H

#include <QGroupBox>
#include <QTimer>

#include "utils/universe.h"
#include "station.h"

namespace Ui {
    class QSunInfo;
}

class QSunInfo: public QGroupBox {
    Q_OBJECT
private:
    Ui::QSunInfo *ui;
    const Station* m_station;
    QTimer *m_timer_short, *m_timer_long;
public:
    explicit QSunInfo(QWidget *parent = nullptr);
    ~QSunInfo();
    void set_universe(const Universe *universe);
    void set_station(const Station *station);

public slots:
    void update_short_term(void);
    void update_long_term(void);
};

#endif // SUNINFO_H
