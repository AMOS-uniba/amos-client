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
public:
    explicit QSightingBuffer(QWidget * parent = nullptr);
    ~QSightingBuffer();

    inline QSightingModel * model(void) { return this->m_sighting_model; }
};

#endif // QSIGHTINGBUFFER_H
