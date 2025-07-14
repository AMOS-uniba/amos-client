#ifndef QSIGHTINGBUFFER_H
#define QSIGHTINGBUFFER_H

#include <QGroupBox>
#include <QNetworkReply>

#include "utils/sighting.h"

QT_FORWARD_DECLARE_CLASS(QSightingModel);

namespace Ui {
    class QSightingBuffer;
}

class QSightingBuffer: public QGroupBox {
    Q_OBJECT
private:
    Ui::QSightingBuffer * ui;
    QSightingModel * m_sighting_model;

    QDateTime m_last_data;
    QTimer * m_reload_timer;

private slots:
    void display_time(void);

public:
    explicit QSightingBuffer(QWidget * parent = nullptr);
    ~QSightingBuffer();

    inline QSightingModel * model(void) { return this->m_sighting_model; }

public slots:
    void handle_sightings_scanned(void);
};

#endif // QSIGHTINGBUFFER_H
