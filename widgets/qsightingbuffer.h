#ifndef QSIGHTINGBUFFER_H
#define QSIGHTINGBUFFER_H

#include <QGroupBox>

#include "utils/sighting.h"

namespace Ui {
    class QSightingBuffer;
}

class QSightingBuffer: public QGroupBox {
    Q_OBJECT
private:
    Ui::QSightingBuffer * ui;
    QMap<QString, Sighting> m_sightings;
public:
    explicit QSightingBuffer(QWidget * parent = nullptr);
    ~QSightingBuffer();
public slots:
    void insert(const Sighting & sighting);
    void remove(const QString & sighting_id);
    void defer(const QString & sighting_id);
};

#endif // QSIGHTINGBUFFER_H
